# Elevator System — LLD Interview Notes

These notes follow your design, with the refinements we discussed. The goal is an interview-ready implementation that starts simple and can be extended when the interviewer adds requirements.

## 1. Problem statement

Design an elevator system for a building with N floors and M lifts.

The system should support passengers requesting a lift from a floor using UP/DOWN buttons, selecting a destination from inside a lift, assigning hall requests to suitable lifts, moving lifts independently, stopping at scheduled floors, and opening and closing doors safely.

We will also discuss duplicate requests, capacity, concurrency, starvation, and what happens when a lift becomes unavailable.

Scope for the initial implementation: In-memory system with one worker per lift. We do not need a database, distributed coordination, or the State Pattern unless the requirements expand.

## 2. Your core entities

C++

```
enum class Direction {
    UP,
    DOWN
};

enum class LiftStatus {
    IDLE,
    MOVING_UP,
    MOVING_DOWN,
    OUT_OF_SERVICE
};

enum class DoorStatus {
    OPEN,
    CLOSED,
    OPENING,
    CLOSING
};

class Floor {
public:
    int number;
};

class Lift {
public:
    int id;
    int currentFloor;
    int capacity;
    int currentOccupancy;

    LiftStatus status;
    DoorStatus doorStatus;
};

class Building {
public:
    int id;
    string name;

    vector<Floor> floors;
    vector<Lift> lifts;
};
```

Design decision: Keep each lift's current floor, movement status, door status, and capacity together. `LiftService` operates on a particular lift; it should not have one shared status field for all lifts.

You do not need a separate `LiftStore` just to duplicate the lifts already held by `Building`. A store becomes useful if you later need independent lookup or persistence.

## 3. Services and responsibilities

|
Component

|

Responsibility

|
| --- | --- |
|

`ElevatorApp`

|

Entry point for passenger actions

|
|

`LiftManagementService`

|

Handles hall requests, prevents duplicate assignment, selects a lift, and schedules pickup

|
|

`RequestLiftStrategy`

|

Chooses which lift should serve a hall request

|
|

`LiftService`

|

Handles internal buttons, worker execution, movement, and doors for an individual lift

|
|

`LiftMovementStrategy`

|

Chooses which stop/direction a particular lift should serve next

|
|

`LiftOperationStore`

|

Stores pending stops for each lift

|

The important distinction is:

`RequestLiftStrategy` selects a lift. `LiftMovementStrategy` selects that lift's next movement. `LiftService` executes the movement.

An internal destination button does not need `RequestLiftStrategy`: the passenger is already inside a specific lift.

## 4. Lift operations: your two-set approach

Your original idea was a queue of floors per lift. We changed it to two ordered sets because stops should be served in floor order, not request-arrival order.

C++

```
struct PendingStops {
    set<int> upStops;
    set<int> downStops;
};

unordered_map<int, PendingStops> operationsByLiftId;
```

`upStops` contains floors scheduled for the lift's UP journey; `downStops` contains floors scheduled for its DOWN journey.

Example:

```
Current floor: 2
Current direction: UP

upStops   = {5, 8}
downStops = {1, 4}

Route: 2 → 5 → 8 → 4 → 1
```

If floor 6 is added while the lift is moving UP and has not passed it, the UP journey becomes `5 → 6 → 8`. This is why a FIFO queue is not suitable.

### Finding the next stop

C++

```
optional<int> nextUpStop(const set<int>& upStops, int currentFloor) {
    auto it = upStops.upper_bound(currentFloor);

    if (it == upStops.end()) {
        return nullopt;
    }

    return *it;
}

optional<int> nextDownStop(const set<int>& downStops, int currentFloor) {
    auto it = downStops.lower_bound(currentFloor);

    if (it == downStops.begin()) {
        return nullopt;
    }

    return *prev(it);
}
```

Check whether the current floor itself is scheduled before looking for the next floor.

|
Operation

|

Complexity

|
| --- | --- |
|

Insert a stop

|

`O(log S)`

|
|

Find next stop

|

`O(log S)`

|
|

Check whether a stop exists

|

`O(log S)`

|
|

Remove a stop by key

|

`O(log S)`

|

Here, `S` is the number of pending stops for that lift.

Important modeling detail: These sets represent the journey on which a stop will be served. A cabin destination has no passenger-requested UP/DOWN direction: its placement depends on whether the lift can still reach that floor during its current journey.

## 5. Hall request versus cabin request

These are two different operations.

### Hall request

A passenger at floor 4 presses UP:

C++

```
liftManagementService.requestLift(
    buildingId,
    4,
    Direction::UP
);
```

Flow:

```
Hall button pressed
       ↓
Check whether floor 4 UP is already pending/assigned
       ↓
RequestLiftStrategy selects a suitable lift
       ↓
Assign request to that lift
       ↓
Schedule pickup stop in LiftOperationStore
       ↓
Notify that lift's worker
```

### Cabin request

A passenger inside lift E1 presses floor 8:

C++

```
liftService.pressLiftFloorButton(liftId, 8);
```

Flow:

```
Cabin button pressed
       ↓
Check whether destination is valid
       ↓
Add destination to E1's appropriate stop set
       ↓
Notify E1's worker
```

The hall-request registry and stop sets solve different problems. The registry tracks who owns a pending pickup request. The stop sets tell each lift where to stop.

## 6. Hall-request registry: avoiding duplicate assignments

You asked: If floor 4 UP is already assigned to a lift, should we assign it again?

No. The management service should check for an existing pending or assigned request before selecting another lift.

C++

```
enum class RequestStatus {
    PENDING,
    ASSIGNED,
    SERVED
};

struct HallRequest {
    int floor;
    Direction direction;
    optional<int> assignedLiftId;
    RequestStatus status;
};

// Key = (floor, requested direction)
map<pair<int, Direction>, HallRequest> hallRequests;
```

Two passengers pressing floor 4 UP can share one pending pickup request. But floor 4 UP and floor 4 DOWN are distinct requests.

The management service must synchronize the check + create + assign sequence. Otherwise, two simultaneous button presses could both observe “not assigned” and assign two lifts.

When an assigned lift becomes unavailable or cannot take passengers because it is full, keep the request pending and reassign it. Do not mark it served merely because the lift reached that floor.

Refinement for a fuller implementation: A single `set<int>` entry cannot tell you why the lift is stopping. If the lift has both a cabin destination at floor 4 and a pending floor 4 DOWN pickup, stopping there on the UP journey must not clear the DOWN pickup. Keep the hall-request registry as the source of truth for pickup direction and completion.

## 7. Lift assignment strategy

Your answer was correct: absolute floor distance alone is insufficient.

Consider:

```
E1: floor 2, moving UP, stops at 5 and 8
E2: floor 7, moving DOWN, stop at 3
E3: floor 1, IDLE

Passenger: floor 4, wants UP
```

The strategy should consider the lift's direction, current floor, pending stops, idle/moving state, capacity, and estimated time to reach the passenger.

For example, E1 is geographically close and moving toward floor 4, but an idle lift might arrive sooner in another scenario if E1 has many intermediate stops.

C++

```
class RequestLiftStrategy {
public:
    virtual ~RequestLiftStrategy() = default;

    virtual optional<int> findLift(
        const Building& building,
        int pickupFloor,
        Direction requestedDirection
    ) = 0;
};
```

Start with a simple direction-compatible nearest-lift strategy. If the interviewer asks for optimization, discuss estimated pickup time rather than claiming that nearest distance is always optimal.

## 8. Lift worker: independent execution

Each lift has its own worker. A passenger button press schedules work; it should not directly move the lift or open its doors.

```
Lift worker wakes up
       ↓
Check pending stops and current lift state
       ↓
Choose next direction/stop
       ↓
Ensure doors are closed
       ↓
Move one floor
       ↓
Check whether current floor should be served
       ↓
Stop → open doors → allow boarding/exiting → close doors
       ↓
Update completed stops and requests
       ↓
Repeat
```

Move one floor at a time in the simulation and recheck requests. If E1 is travelling from floor 2 toward floor 8 and floor 4 is added while E1 is at floor 3, it can incorporate floor 4 without waiting to reach floor 8.

When there is no work, the worker should wait on a condition variable, not continuously poll.

C++

```
unique_lock<mutex> lock(liftMutex);

cv.wait(lock, [&] {
    return hasPendingWork(liftId) || shutdownRequested;
});
```

A request handler inserts work under the appropriate lock, then notifies the worker.

## 9. Concurrency: uniqueness is not thread safety

Your answer was that a `set` naturally deduplicates requests. That is true, but `std::set` does not support concurrent reads and writes without synchronization.

For an in-memory implementation, a straightforward design is:

C++

```
struct LiftRuntime {
    mutex mtx;
    condition_variable cv;

    PendingStops stops;
    Lift lift;
};
```

The request handler locks the relevant lift runtime, inserts a stop, unlocks, and notifies the worker. The worker locks when inspecting or modifying the stop sets and lift state.

If you use a genuinely thread-safe container, you do not need an extra mutex solely for its individual insert/find/erase operations. But you may still need synchronization for a multi-step operation involving both the stops and the lift's state.

For example, “check current direction → decide which set to insert into → insert stop” should use a consistent view of the direction. Individually thread-safe operations do not automatically make that whole sequence atomic.

Do not hold the mutex throughout a simulated trip or while waiting for passengers to board. Lock only the shared-state operations that must be consistent.

## 10. Doors and the State Pattern

You proposed:

```
LiftState
 ├── MovingLiftState
 ├── IdleLiftState
 ├── DoorOpenLiftState
 ├── DoorClosedLiftState
 └── NonFunctionalLiftState
```

This is a reasonable direction if the interviewer asks you to apply the State Pattern. But you do not need it in the initial elevator design.

Start with `LiftStatus` and `DoorStatus` enums and explicit checks in `LiftService`:

C++

```
void LiftService::moveLift(Lift& lift, Direction direction) {
    if (lift.status == LiftStatus::OUT_OF_SERVICE) {
        throw logic_error("Lift unavailable");
    }

    if (lift.doorStatus != DoorStatus::CLOSED) {
        throw logic_error("Cannot move with doors open");
    }

    // Begin movement and update status.
}
```

A typical sequence is:

```
Stopped, doors open
       ↓
Close doors
       ↓
Confirm doors are closed
       ↓
Start movement
       ↓
Stop at scheduled floor
       ↓
Open doors
```

If door closing fails or an obstruction is detected, do not start movement.

### When to introduce State Pattern

Consider it if the interviewer adds many state-dependent behaviors: door obstruction, emergency stop, maintenance mode, fire mode, and complex rules about which operations are allowed.

If you do introduce it, let each `Lift` own its current state. `LiftService` coordinates operations on that lift. Avoid putting one shared `LiftState` inside a service that manages all lifts.

Interview explanation: “I'll start with enums and explicit transition checks. If state-specific behavior grows, I can refactor it into the State Pattern.”

## 11. Other important interviewer follow-ups

|
Question

|

Expected design discussion

|
| --- | --- |
|

What if a lift becomes full before pickup?

|

Keep the hall request pending; reassign it to another suitable lift.

|
|

What if several passengers press the same button?

|

One pending stop/request per floor and direction is sufficient for the basic model.

|
|

What if UP requests keep arriving?

|

Use a scheduling policy that eventually reverses direction; do not indefinitely postpone existing DOWN stops.

|
|

What if a lift becomes unavailable?

|

Mark it out of service, stop assigning new work, and reassign its outstanding hall requests.

|
|

What if a request arrives while doors are open?

|

Add it to pending work; the worker still closes and confirms the doors before moving.

|
|

What if two lifts try to claim the same hall request?

|

Synchronize assignment in `LiftManagementService` and its hall-request registry.

|
|

What if there are no pending stops?

|

Worker waits on a condition variable and wakes on new work or shutdown.

|

## 12. Class diagram

Diagram options

![](data\:image/svg+xml;utf8,%3Csvg%20id%3D%22mermaid-_r_44t_%22%20width%3D%221587.608154296875%22%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20class%3D%22classDiagram%22%20height%3D%22668.4000244140625%22%20viewBox%3D%224%204%201587.608154296875%20668.4000244140625%22%20role%3D%22graphics-document%20document%22%20aria-roledescription%3D%22class%22%3E%3Cstyle%3E%23mermaid-_r_44t_%7Bfont-family%3A%22-apple-system%22%2C%22BlinkMacSystemFont%22%2C%22Segoe%20UI%22%2C%22Roboto%22%2C%22Oxygen%22%2C%22Ubuntu%22%2C%22Cantarell%22%2C%22Helvetica%20Neue%22%2C%22Arial%22%2C%22sans-serif%22%3Bfont-size%3A14px%3Bfill%3Argb\(255%2C%20255%2C%20255\)%3B%7D%40keyframes%20edge-animation-frame%7Bfrom%7Bstroke-dashoffset%3A0%3B%7D%7D%40keyframes%20dash%7Bto%7Bstroke-dashoffset%3A0%3B%7D%7D%23mermaid-_r_44t_%20.edge-animation-slow%7Bstroke-dasharray%3A9%2C5!important%3Bstroke-dashoffset%3A900%3Banimation%3Adash%2050s%20linear%20infinite%3Bstroke-linecap%3Around%3B%7D%23mermaid-_r_44t_%20.edge-animation-fast%7Bstroke-dasharray%3A9%2C5!important%3Bstroke-dashoffset%3A900%3Banimation%3Adash%2020s%20linear%20infinite%3Bstroke-linecap%3Around%3B%7D%23mermaid-_r_44t_%20.error-icon%7Bfill%3Argb\(33%2C%2033%2C%2033\)%3B%7D%23mermaid-_r_44t_%20.error-text%7Bfill%3Argb\(255%2C%20255%2C%20255\)%3Bstroke%3Argb\(255%2C%20255%2C%20255\)%3B%7D%23mermaid-_r_44t_%20.edge-thickness-normal%7Bstroke-width%3A1px%3B%7D%23mermaid-_r_44t_%20.edge-thickness-thick%7Bstroke-width%3A3.5px%3B%7D%23mermaid-_r_44t_%20.edge-pattern-solid%7Bstroke-dasharray%3A0%3B%7D%23mermaid-_r_44t_%20.edge-thickness-invisible%7Bstroke-width%3A0%3Bfill%3Anone%3B%7D%23mermaid-_r_44t_%20.edge-pattern-dashed%7Bstroke-dasharray%3A3%3B%7D%23mermaid-_r_44t_%20.edge-pattern-dotted%7Bstroke-dasharray%3A2%3B%7D%23mermaid-_r_44t_%20.marker%7Bfill%3Argb\(205%2C%20205%2C%20205\)%3Bstroke%3Argb\(205%2C%20205%2C%20205\)%3B%7D%23mermaid-_r_44t_%20.marker.cross%7Bstroke%3Argb\(205%2C%20205%2C%20205\)%3B%7D%23mermaid-_r_44t_%20svg%7Bfont-family%3A%22-apple-system%22%2C%22BlinkMacSystemFont%22%2C%22Segoe%20UI%22%2C%22Roboto%22%2C%22Oxygen%22%2C%22Ubuntu%22%2C%22Cantarell%22%2C%22Helvetica%20Neue%22%2C%22Arial%22%2C%22sans-serif%22%3Bfont-size%3A14px%3B%7D%23mermaid-_r_44t_%20p%7Bmargin%3A0%3B%7D%23mermaid-_r_44t_%20g.classGroup%20text%7Bfill%3Argb\(31%2C%2078%2C%20148\)%3Bstroke%3Anone%3Bfont-family%3A%22-apple-system%22%2C%22BlinkMacSystemFont%22%2C%22Segoe%20UI%22%2C%22Roboto%22%2C%22Oxygen%22%2C%22Ubuntu%22%2C%22Cantarell%22%2C%22Helvetica%20Neue%22%2C%22Arial%22%2C%22sans-serif%22%3Bfont-size%3A10px%3B%7D%23mermaid-_r_44t_%20g.classGroup%20text%20.title%7Bfont-weight%3Abolder%3B%7D%23mermaid-_r_44t_%20.nodeLabel%2C%23mermaid-_r_44t_%20.edgeLabel%7Bcolor%3Argb\(255%2C%20255%2C%20255\)%3B%7D%23mermaid-_r_44t_%20.edgeLabel%20.label%20rect%7Bfill%3Argb\(9%2C%2023%2C%2044\)%3B%7D%23mermaid-_r_44t_%20.label%20text%7Bfill%3Argb\(255%2C%20255%2C%20255\)%3B%7D%23mermaid-_r_44t_%20.labelBkg%7Bbackground%3Argb\(9%2C%2023%2C%2044\)%3B%7D%23mermaid-_r_44t_%20.edgeLabel%20.label%20span%7Bbackground%3Argb\(9%2C%2023%2C%2044\)%3B%7D%23mermaid-_r_44t_%20.classTitle%7Bfont-weight%3Abolder%3B%7D%23mermaid-_r_44t_%20.node%20rect%2C%23mermaid-_r_44t_%20.node%20circle%2C%23mermaid-_r_44t_%20.node%20ellipse%2C%23mermaid-_r_44t_%20.node%20polygon%2C%23mermaid-_r_44t_%20.node%20path%7Bfill%3Argb\(9%2C%2023%2C%2044\)%3Bstroke%3Argb\(31%2C%2078%2C%20148\)%3Bstroke-width%3A1px%3B%7D%23mermaid-_r_44t_%20.divider%7Bstroke%3Argb\(31%2C%2078%2C%20148\)%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20g.clickable%7Bcursor%3Apointer%3B%7D%23mermaid-_r_44t_%20g.classGroup%20rect%7Bfill%3Argb\(9%2C%2023%2C%2044\)%3Bstroke%3Argb\(31%2C%2078%2C%20148\)%3B%7D%23mermaid-_r_44t_%20g.classGroup%20line%7Bstroke%3Argb\(31%2C%2078%2C%20148\)%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20.classLabel%20.box%7Bstroke%3Anone%3Bstroke-width%3A0%3Bfill%3Argb\(9%2C%2023%2C%2044\)%3Bopacity%3A0.5%3B%7D%23mermaid-_r_44t_%20.classLabel%20.label%7Bfill%3Argb\(31%2C%2078%2C%20148\)%3Bfont-size%3A10px%3B%7D%23mermaid-_r_44t_%20.relation%7Bstroke%3Argb\(205%2C%20205%2C%20205\)%3Bstroke-width%3A1%3Bfill%3Anone%3B%7D%23mermaid-_r_44t_%20.dashed-line%7Bstroke-dasharray%3A3%3B%7D%23mermaid-_r_44t_%20.dotted-line%7Bstroke-dasharray%3A1%202%3B%7D%23mermaid-_r_44t_%20%23compositionStart%2C%23mermaid-_r_44t_%20.composition%7Bfill%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23compositionEnd%2C%23mermaid-_r_44t_%20.composition%7Bfill%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23dependencyStart%2C%23mermaid-_r_44t_%20.dependency%7Bfill%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23dependencyStart%2C%23mermaid-_r_44t_%20.dependency%7Bfill%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23extensionStart%2C%23mermaid-_r_44t_%20.extension%7Bfill%3Atransparent!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23extensionEnd%2C%23mermaid-_r_44t_%20.extension%7Bfill%3Atransparent!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23aggregationStart%2C%23mermaid-_r_44t_%20.aggregation%7Bfill%3Atransparent!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23aggregationEnd%2C%23mermaid-_r_44t_%20.aggregation%7Bfill%3Atransparent!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23lollipopStart%2C%23mermaid-_r_44t_%20.lollipop%7Bfill%3Argb\(9%2C%2023%2C%2044\)!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20%23lollipopEnd%2C%23mermaid-_r_44t_%20.lollipop%7Bfill%3Argb\(9%2C%2023%2C%2044\)!important%3Bstroke%3Argb\(205%2C%20205%2C%20205\)!important%3Bstroke-width%3A1%3B%7D%23mermaid-_r_44t_%20.edgeTerminals%7Bfont-size%3A11px%3Bline-height%3Ainitial%3B%7D%23mermaid-_r_44t_%20.classTitleText%7Btext-anchor%3Amiddle%3Bfont-size%3A18px%3Bfill%3Argb\(255%2C%20255%2C%20255\)%3B%7D%23mermaid-_r_44t_%20.label-icon%7Bdisplay%3Ainline-block%3Bheight%3A1em%3Boverflow%3Avisible%3Bvertical-align%3A-0.125em%3B%7D%23mermaid-_r_44t_%20.node%20.label-icon%20path%7Bfill%3AcurrentColor%3Bstroke%3Arevert%3Bstroke-width%3Arevert%3B%7D%23mermaid-_r_44t_%20.node%7Bcolor-scheme%3Adark%3B%7D%23mermaid-_r_44t_%20%3Aroot%7B--mermaid-font-family%3A%22-apple-system%22%2C%22BlinkMacSystemFont%22%2C%22Segoe%20UI%22%2C%22Roboto%22%2C%22Oxygen%22%2C%22Ubuntu%22%2C%22Cantarell%22%2C%22Helvetica%20Neue%22%2C%22Arial%22%2C%22sans-serif%22%3B%7D%3C%2Fstyle%3E%3Cg%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-aggregationStart%22%20class%3D%22marker%20aggregation%20class%22%20refX%3D%2218%22%20refY%3D%227%22%20markerWidth%3D%22190%22%20markerHeight%3D%22240%22%20orient%3D%22auto%22%3E%3Cpath%20d%3D%22M%2018%2C7%20L9%2C13%20L1%2C7%20L9%2C1%20Z%22%3E%3C%2Fpath%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-aggregationEnd%22%20class%3D%22marker%20aggregation%20class%22%20refX%3D%221%22%20refY%3D%227%22%20markerWidth%3D%2220%22%20markerHeight%3D%2228%22%20orient%3D%22auto%22%3E%3Cpath%20d%3D%22M%2018%2C7%20L9%2C13%20L1%2C7%20L9%2C1%20Z%22%3E%3C%2Fpath%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-extensionStart%22%20class%3D%22marker%20extension%20class%22%20refX%3D%2218%22%20refY%3D%227%22%20markerWidth%3D%22190%22%20markerHeight%3D%22240%22%20orient%3D%22auto%22%3E%3Cpath%20d%3D%22M%201%2C7%20L18%2C13%20V%201%20Z%22%3E%3C%2Fpath%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-extensionEnd%22%20class%3D%22marker%20extension%20class%22%20refX%3D%221%22%20refY%3D%227%22%20markerWidth%3D%2220%22%20markerHeight%3D%2228%22%20orient%3D%22auto%22%3E%3Cpath%20d%3D%22M%201%2C1%20V%2013%20L18%2C7%20Z%22%3E%3C%2Fpath%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-compositionStart%22%20class%3D%22marker%20composition%20class%22%20refX%3D%2218%22%20refY%3D%227%22%20markerWidth%3D%22190%22%20markerHeight%3D%22240%22%20orient%3D%22auto%22%3E%3Cpath%20d%3D%22M%2018%2C7%20L9%2C13%20L1%2C7%20L9%2C1%20Z%22%3E%3C%2Fpath%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-compositionEnd%22%20class%3D%22marker%20composition%20class%22%20refX%3D%221%22%20refY%3D%227%22%20markerWidth%3D%2220%22%20markerHeight%3D%2228%22%20orient%3D%22auto%22%3E%3Cpath%20d%3D%22M%2018%2C7%20L9%2C13%20L1%2C7%20L9%2C1%20Z%22%3E%3C%2Fpath%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-dependencyStart%22%20class%3D%22marker%20dependency%20class%22%20refX%3D%226%22%20refY%3D%227%22%20markerWidth%3D%22190%22%20markerHeight%3D%22240%22%20orient%3D%22auto%22%3E%3Cpath%20d%3D%22M%205%2C7%20L9%2C13%20L1%2C7%20L9%2C1%20Z%22%3E%3C%2Fpath%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-dependencyEnd%22%20class%3D%22marker%20dependency%20class%22%20refX%3D%2213%22%20refY%3D%227%22%20markerWidth%3D%2220%22%20markerHeight%3D%2228%22%20orient%3D%22auto%22%3E%3Cpath%20d%3D%22M%2018%2C7%20L9%2C13%20L14%2C7%20L9%2C1%20Z%22%3E%3C%2Fpath%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-lollipopStart%22%20class%3D%22marker%20lollipop%20class%22%20refX%3D%2213%22%20refY%3D%227%22%20markerWidth%3D%22190%22%20markerHeight%3D%22240%22%20orient%3D%22auto%22%3E%3Ccircle%20stroke%3D%22black%22%20fill%3D%22transparent%22%20cx%3D%227%22%20cy%3D%227%22%20r%3D%226%22%3E%3C%2Fcircle%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3Cdefs%3E%3Cmarker%20id%3D%22mermaid-_r_44t__class-lollipopEnd%22%20class%3D%22marker%20lollipop%20class%22%20refX%3D%221%22%20refY%3D%227%22%20markerWidth%3D%22190%22%20markerHeight%3D%22240%22%20orient%3D%22auto%22%3E%3Ccircle%20stroke%3D%22black%22%20fill%3D%22transparent%22%20cx%3D%227%22%20cy%3D%227%22%20r%3D%226%22%3E%3C%2Fcircle%3E%3C%2Fmarker%3E%3C%2Fdefs%3E%3C%2Fg%3E%3Cg%20class%3D%22subgraphs%22%3E%3C%2Fg%3E%3Cg%20class%3D%22nodes%22%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-Building-38%22%20transform%3D%22translate\(106.7400894165039%2C%20302.1000061035156\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-94.7400894165039%20-86.5%20L94.7400894165039%20-86.5%20L94.7400894165039%2086.5%20L-94.7400894165039%2086.5%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-94.7400894165039%20-86.5%20C-34.6590108840569%20-86.5%2C%2025.42206764839011%20-86.5%2C%2094.7400894165039%20-86.5%20M-94.7400894165039%20-86.5%20C-24.15251718631633%20-86.5%2C%2046.435055043871245%20-86.5%2C%2094.7400894165039%20-86.5%20M94.7400894165039%20-86.5%20C94.7400894165039%20-49.352984050144315%2C%2094.7400894165039%20-12.20596810028863%2C%2094.7400894165039%2086.5%20M94.7400894165039%20-86.5%20C94.7400894165039%20-39.25499022584667%2C%2094.7400894165039%207.9900195483066625%2C%2094.7400894165039%2086.5%20M94.7400894165039%2086.5%20C20.114600325540735%2086.5%2C%20-54.510888765422436%2086.5%2C%20-94.7400894165039%2086.5%20M94.7400894165039%2086.5%20C51.35524217706033%2086.5%2C%207.97039493761676%2086.5%2C%20-94.7400894165039%2086.5%20M-94.7400894165039%2086.5%20C-94.7400894165039%2030.462150151379127%2C%20-94.7400894165039%20-25.575699697241745%2C%20-94.7400894165039%20-86.5%20M-94.7400894165039%2086.5%20C-94.7400894165039%2035.47485494512697%2C%20-94.7400894165039%20-15.550290109746058%2C%20-94.7400894165039%20-86.5%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(0%2C%20-66.5\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-27.659276962280273%2C%20-66.5\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3EBuilding%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-82.7400894165039%2C%20-24.100000381469727\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2Bint%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20id%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C12.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2Bstring%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20name%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C33.59999942779541\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%2Bvector%26lt%3BFloor%26gt%3B%20floors%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C54.999999046325684\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%2Bvector%26lt%3BLift%26gt%3B%20lifts%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-82.7400894165039%2C%2082.49999809265137\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-94.7400894165039%20-44.10000038146973%20C-36.50857069024423%20-44.10000038146973%2C%2021.722948036015453%20-44.10000038146973%2C%2094.7400894165039%20-44.10000038146973%20M-94.7400894165039%20-44.10000038146973%20C-41.581586516259506%20-44.10000038146973%2C%2011.576916383984894%20-44.10000038146973%2C%2094.7400894165039%20-44.10000038146973%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-94.7400894165039%2062.49999809265137%20C-47.22762404686123%2062.49999809265137%2C%200.28484132278144614%2062.49999809265137%2C%2094.7400894165039%2062.49999809265137%20M-94.7400894165039%2062.49999809265137%20C-54.2173491831788%2062.49999809265137%2C%20-13.694608949853688%2062.49999809265137%2C%2094.7400894165039%2062.49999809265137%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-Lift-39%22%20transform%3D%22translate\(137.50589497884107%2C%20556.5\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-92.29741668701172%20-107.89999389648438%20L92.29741668701172%20-107.89999389648438%20L92.29741668701172%20107.89999389648438%20L-92.29741668701172%20107.89999389648438%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-92.29741668701172%20-107.89999389648438%20C-28.38003426740849%20-107.89999389648438%2C%2035.53734815219474%20-107.89999389648438%2C%2092.29741668701172%20-107.89999389648438%20M-92.29741668701172%20-107.89999389648438%20C-34.41720464202971%20-107.89999389648438%2C%2023.4630074029523%20-107.89999389648438%2C%2092.29741668701172%20-107.89999389648438%20M92.29741668701172%20-107.89999389648438%20C92.29741668701172%20-45.809785516279774%2C%2092.29741668701172%2016.280422863924827%2C%2092.29741668701172%20107.89999389648438%20M92.29741668701172%20-107.89999389648438%20C92.29741668701172%20-53.84947115240634%2C%2092.29741668701172%200.20105159167169973%2C%2092.29741668701172%20107.89999389648438%20M92.29741668701172%20107.89999389648438%20C21.178972921613237%20107.89999389648438%2C%20-49.939470843785244%20107.89999389648438%2C%20-92.29741668701172%20107.89999389648438%20M92.29741668701172%20107.89999389648438%20C47.606740124767%20107.89999389648438%2C%202.9160635625222824%20107.89999389648438%2C%20-92.29741668701172%20107.89999389648438%20M-92.29741668701172%20107.89999389648438%20C-92.29741668701172%2055.75147038265043%2C%20-92.29741668701172%203.6029468688164883%2C%20-92.29741668701172%20-107.89999389648438%20M-92.29741668701172%20107.89999389648438%20C-92.29741668701172%2041.67798267476529%2C%20-92.29741668701172%20-24.544028546953797%2C%20-92.29741668701172%20-107.89999389648438%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(0%2C%20-87.89999389648438\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-11.184277534484863%2C%20-87.89999389648438\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3ELift%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-80.29741668701172%2C%20-45.4999942779541\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2Bint%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20id%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C12.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2Bint%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20currentFloor%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C33.59999942779541\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2Bint%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20capacity%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C54.999999046325684\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2Bint%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20currentOccupancy%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C76.39999866485596\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BLiftStatus%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20status%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C97.79999828338623\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BDoorStatus%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20doorStatus%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-80.29741668701172%2C%20103.9000072479248\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-92.29741668701172%20-65.4999942779541%20C-51.23441905638568%20-65.4999942779541%2C%20-10.171421425759647%20-65.4999942779541%2C%2092.29741668701172%20-65.4999942779541%20M-92.29741668701172%20-65.4999942779541%20C-22.732484882922705%20-65.4999942779541%2C%2046.83244692116631%20-65.4999942779541%2C%2092.29741668701172%20-65.4999942779541%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-92.29741668701172%2083.9000072479248%20C-46.260097116537956%2083.9000072479248%2C%20-0.22277754606419364%2083.9000072479248%2C%2092.29741668701172%2083.9000072479248%20M-92.29741668701172%2083.9000072479248%20C-40.24580645785344%2083.9000072479248%2C%2011.805803771304838%2083.9000072479248%2C%2092.29741668701172%2083.9000072479248%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-ElevatorApp-40%22%20transform%3D%22translate\(1317.0588912963865%2C%2080.10000610351562\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-127.70298767089844%20-68.10000228881836%20L127.70298767089844%20-68.10000228881836%20L127.70298767089844%2068.10000228881836%20L-127.70298767089844%2068.10000228881836%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-127.70298767089844%20-68.10000228881836%20C-59.89368660882863%20-68.10000228881836%2C%207.915614453241176%20-68.10000228881836%2C%20127.70298767089844%20-68.10000228881836%20M-127.70298767089844%20-68.10000228881836%20C-30.914178404052393%20-68.10000228881836%2C%2065.87463086279365%20-68.10000228881836%2C%20127.70298767089844%20-68.10000228881836%20M127.70298767089844%20-68.10000228881836%20C127.70298767089844%20-39.24715974092244%2C%20127.70298767089844%20-10.394317193026524%2C%20127.70298767089844%2068.10000228881836%20M127.70298767089844%20-68.10000228881836%20C127.70298767089844%20-17.433410664648378%2C%20127.70298767089844%2033.233180959521604%2C%20127.70298767089844%2068.10000228881836%20M127.70298767089844%2068.10000228881836%20C59.12322928708019%2068.10000228881836%2C%20-9.456529096738052%2068.10000228881836%2C%20-127.70298767089844%2068.10000228881836%20M127.70298767089844%2068.10000228881836%20C39.19458115213362%2068.10000228881836%2C%20-49.31382536663119%2068.10000228881836%2C%20-127.70298767089844%2068.10000228881836%20M-127.70298767089844%2068.10000228881836%20C-127.70298767089844%2026.574558724314805%2C%20-127.70298767089844%20-14.950884840188749%2C%20-127.70298767089844%20-68.10000228881836%20M-127.70298767089844%2068.10000228881836%20C-127.70298767089844%2023.66377397486695%2C%20-127.70298767089844%20-20.77245433908446%2C%20-127.70298767089844%20-68.10000228881836%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(0%2C%20-48.10000228881836\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-40.380958557128906%2C%20-48.10000228881836\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3EElevatorApp%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-115.70298767089844%2C%20-5.700002670288086\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-115.70298767089844%2C%2024.299997329711914\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BrequestLift\(floor%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20direction\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C12.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BpressCabinButton\(liftId%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20floor\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-127.70298767089844%20-25.700002670288086%20C-26.325054207312775%20-25.700002670288086%2C%2075.05287925627289%20-25.700002670288086%2C%20127.70298767089844%20-25.700002670288086%20M-127.70298767089844%20-25.700002670288086%20C-32.02784550784554%20-25.700002670288086%2C%2063.647296655207356%20-25.700002670288086%2C%20127.70298767089844%20-25.700002670288086%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-127.70298767089844%20-1.700002670288086%20C-35.607933010644444%20-1.700002670288086%2C%2056.48712164960955%20-1.700002670288086%2C%20127.70298767089844%20-1.700002670288086%20M-127.70298767089844%20-1.700002670288086%20C-73.81646882666936%20-1.700002670288086%2C%20-19.929949982440277%20-1.700002670288086%2C%20127.70298767089844%20-1.700002670288086%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-LiftManagementService-41%22%20transform%3D%22translate\(1359.6265538533526%2C%20299.1000061035156\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-149.106201171875%20-78.80000305175781%20L149.106201171875%20-78.80000305175781%20L149.106201171875%2078.80000305175781%20L-149.106201171875%2078.80000305175781%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-149.106201171875%20-78.80000305175781%20C-35.861006847334465%20-78.80000305175781%2C%2077.38418747720607%20-78.80000305175781%2C%20149.106201171875%20-78.80000305175781%20M-149.106201171875%20-78.80000305175781%20C-69.91976994476724%20-78.80000305175781%2C%209.266661282340522%20-78.80000305175781%2C%20149.106201171875%20-78.80000305175781%20M149.106201171875%20-78.80000305175781%20C149.106201171875%20-19.575760787218783%2C%20149.106201171875%2039.648481477320246%2C%20149.106201171875%2078.80000305175781%20M149.106201171875%20-78.80000305175781%20C149.106201171875%20-40.954496485537305%2C%20149.106201171875%20-3.108989919316798%2C%20149.106201171875%2078.80000305175781%20M149.106201171875%2078.80000305175781%20C87.6794205182583%2078.80000305175781%2C%2026.25263986464158%2078.80000305175781%2C%20-149.106201171875%2078.80000305175781%20M149.106201171875%2078.80000305175781%20C46.56979685308224%2078.80000305175781%2C%20-55.966607465710524%2078.80000305175781%2C%20-149.106201171875%2078.80000305175781%20M-149.106201171875%2078.80000305175781%20C-149.106201171875%2033.85058952985638%2C%20-149.106201171875%20-11.098823992045055%2C%20-149.106201171875%20-78.80000305175781%20M-149.106201171875%2078.80000305175781%20C-149.106201171875%2042.917158647381584%2C%20-149.106201171875%207.034314243005355%2C%20-149.106201171875%20-78.80000305175781%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(0%2C%20-58.80000305175781\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-78.77490234375%2C%20-58.80000305175781\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3ELiftManagementService%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-137.106201171875%2C%20-16.40000343322754\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-137.106201171875%2C%2013.599996566772461\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BrequestLift\(floor%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20direction\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C12.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BreassignRequest\(request\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C33.59999942779541\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BcompleteHallRequest\(request\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-149.106201171875%20-36.40000343322754%20C-75.091194493911%20-36.40000343322754%2C%20-1.0761878159470086%20-36.40000343322754%2C%20149.106201171875%20-36.40000343322754%20M-149.106201171875%20-36.40000343322754%20C-82.02882416085099%20-36.40000343322754%2C%20-14.951447149826976%20-36.40000343322754%2C%20149.106201171875%20-36.40000343322754%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-149.106201171875%20-12.400003433227539%20C-78.99020496119985%20-12.400003433227539%2C%20-8.874208750524701%20-12.400003433227539%2C%20149.106201171875%20-12.400003433227539%20M-149.106201171875%20-12.400003433227539%20C-45.06806227142691%20-12.400003433227539%2C%2058.97007662902118%20-12.400003433227539%2C%20149.106201171875%20-12.400003433227539%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-RequestLiftStrategy-42%22%20transform%3D%22translate\(1434.17965443929%2C%20515.2000045776367\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-149.428466796875%20-66.5999984741211%20L149.428466796875%20-66.5999984741211%20L149.428466796875%2066.5999984741211%20L-149.428466796875%2066.5999984741211%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-149.428466796875%20-66.5999984741211%20C-70.19429668570136%20-66.5999984741211%2C%209.03987342547228%20-66.5999984741211%2C%20149.428466796875%20-66.5999984741211%20M-149.428466796875%20-66.5999984741211%20C-74.13988890748767%20-66.5999984741211%2C%201.148688981899653%20-66.5999984741211%2C%20149.428466796875%20-66.5999984741211%20M149.428466796875%20-66.5999984741211%20C149.428466796875%20-35.903037263897616%2C%20149.428466796875%20-5.206076053674138%2C%20149.428466796875%2066.5999984741211%20M149.428466796875%20-66.5999984741211%20C149.428466796875%20-29.516452006536817%2C%20149.428466796875%207.567094461047461%2C%20149.428466796875%2066.5999984741211%20M149.428466796875%2066.5999984741211%20C54.642371930609%2066.5999984741211%2C%20-40.143722935656996%2066.5999984741211%2C%20-149.428466796875%2066.5999984741211%20M149.428466796875%2066.5999984741211%20C51.8016844248159%2066.5999984741211%2C%20-45.825097947243194%2066.5999984741211%2C%20-149.428466796875%2066.5999984741211%20M-149.428466796875%2066.5999984741211%20C-149.428466796875%2024.7806959927306%2C%20-149.428466796875%20-17.038606488659894%2C%20-149.428466796875%20-66.5999984741211%20M-149.428466796875%2066.5999984741211%20C-149.428466796875%2031.600350733005158%2C%20-149.428466796875%20-3.3992970081107785%2C%20-149.428466796875%20-66.5999984741211%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(-33.91054916381836%2C%20-46.599998474121094\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%C2%ABinterface%C2%BB%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-65.6944351196289%2C%20-28.19999885559082\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3ERequestLiftStrategy%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-137.428466796875%2C%2014.200000762939453\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-137.428466796875%2C%2044.20000076293945\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BfindLift\(building%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20floor%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20direction\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-149.428466796875%20-5.799999237060547%20C-61.95877307136655%20-5.799999237060547%2C%2025.5109206541419%20-5.799999237060547%2C%20149.428466796875%20-5.799999237060547%20M-149.428466796875%20-5.799999237060547%20C-62.4404955358672%20-5.799999237060547%2C%2024.5474757251406%20-5.799999237060547%2C%20149.428466796875%20-5.799999237060547%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-149.428466796875%2018.200000762939453%20C-38.40843952568876%2018.200000762939453%2C%2072.61158774549747%2018.200000762939453%2C%20149.428466796875%2018.200000762939453%20M-149.428466796875%2018.200000762939453%20C-38.4359232632859%2018.200000762939453%2C%2072.5566202703032%2018.200000762939453%2C%20149.428466796875%2018.200000762939453%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-LiftService-43%22%20transform%3D%22translate\(666.9629974365232%2C%20288.40000915527344\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-132.6641616821289%20-100.19999694824219%20L132.6641616821289%20-100.19999694824219%20L132.6641616821289%20100.19999694824219%20L-132.6641616821289%20100.19999694824219%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-132.6641616821289%20-100.19999694824219%20C-53.76317102314084%20-100.19999694824219%2C%2025.137819635847222%20-100.19999694824219%2C%20132.6641616821289%20-100.19999694824219%20M-132.6641616821289%20-100.19999694824219%20C-31.380165904014817%20-100.19999694824219%2C%2069.90382987409927%20-100.19999694824219%2C%20132.6641616821289%20-100.19999694824219%20M132.6641616821289%20-100.19999694824219%20C132.6641616821289%20-45.00898976027427%2C%20132.6641616821289%2010.182017427693651%2C%20132.6641616821289%20100.19999694824219%20M132.6641616821289%20-100.19999694824219%20C132.6641616821289%20-42.214219109056565%2C%20132.6641616821289%2015.771558730129058%2C%20132.6641616821289%20100.19999694824219%20M132.6641616821289%20100.19999694824219%20C29.055775360747205%20100.19999694824219%2C%20-74.5526109606345%20100.19999694824219%2C%20-132.6641616821289%20100.19999694824219%20M132.6641616821289%20100.19999694824219%20C26.730239674071825%20100.19999694824219%2C%20-79.20368233398526%20100.19999694824219%2C%20-132.6641616821289%20100.19999694824219%20M-132.6641616821289%20100.19999694824219%20C-132.6641616821289%2039.28563736473089%2C%20-132.6641616821289%20-21.62872221878041%2C%20-132.6641616821289%20-100.19999694824219%20M-132.6641616821289%20100.19999694824219%20C-132.6641616821289%2047.76540869365523%2C%20-132.6641616821289%20-4.669179560931724%2C%20-132.6641616821289%20-100.19999694824219%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(0%2C%20-80.19999694824219\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-35.0283203125%2C%20-80.19999694824219\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3ELiftService%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-120.6641616821289%2C%20-37.799997329711914\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-120.6641616821289%2C%20-7.799997329711914\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BpressLiftFloorButton\(liftId%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20floor\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C12.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BliftWorkerFunction\(liftId\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C33.59999942779541\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E-moveOneFloor\(liftId%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20direction\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C54.999999046325684\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E-openDoor\(liftId\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C76.39999866485596\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E-closeDoor\(liftId\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-132.66416931152344%20-57.799997329711914%20C-72.2046912362072%20-57.799997329711914%2C%20-11.74521316089097%20-57.799997329711914%2C%20132.66416931152344%20-57.799997329711914%20M-132.66416931152344%20-57.799997329711914%20C-41.58906016532178%20-57.799997329711914%2C%2049.48604898087987%20-57.799997329711914%2C%20132.66416931152344%20-57.799997329711914%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-132.66416931152344%20-33.799997329711914%20C-29.631999036607752%20-33.799997329711914%2C%2073.40017123830793%20-33.799997329711914%2C%20132.66416931152344%20-33.799997329711914%20M-132.66416931152344%20-33.799997329711914%20C-79.00288596136025%20-33.799997329711914%2C%20-25.341602611197075%20-33.799997329711914%2C%20132.66416931152344%20-33.799997329711914%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-LiftMovementStrategy-44%22%20transform%3D%22translate\(422.9353459676105%2C%20515.2000045776367\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-153.1320343017578%20-66.5999984741211%20L153.1320343017578%20-66.5999984741211%20L153.1320343017578%2066.5999984741211%20L-153.1320343017578%2066.5999984741211%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-153.1320343017578%20-66.5999984741211%20C-86.0475390669749%20-66.5999984741211%2C%20-18.963043832192%20-66.5999984741211%2C%20153.1320343017578%20-66.5999984741211%20M-153.1320343017578%20-66.5999984741211%20C-73.52240700456274%20-66.5999984741211%2C%206.087220292632338%20-66.5999984741211%2C%20153.1320343017578%20-66.5999984741211%20M153.1320343017578%20-66.5999984741211%20C153.1320343017578%20-16.975658649576957%2C%20153.1320343017578%2032.64868117496718%2C%20153.1320343017578%2066.5999984741211%20M153.1320343017578%20-66.5999984741211%20C153.1320343017578%20-16.829623823734828%2C%20153.1320343017578%2032.94075082665144%2C%20153.1320343017578%2066.5999984741211%20M153.1320343017578%2066.5999984741211%20C51.7802717209332%2066.5999984741211%2C%20-49.57149085989141%2066.5999984741211%2C%20-153.1320343017578%2066.5999984741211%20M153.1320343017578%2066.5999984741211%20C62.206866211254095%2066.5999984741211%2C%20-28.718301879249623%2066.5999984741211%2C%20-153.1320343017578%2066.5999984741211%20M-153.1320343017578%2066.5999984741211%20C-153.1320343017578%2034.76707484637157%2C%20-153.1320343017578%202.9341512186220484%2C%20-153.1320343017578%20-66.5999984741211%20M-153.1320343017578%2066.5999984741211%20C-153.1320343017578%2031.645946337857744%2C%20-153.1320343017578%20-3.308105798405606%2C%20-153.1320343017578%20-66.5999984741211%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(-33.91054916381836%2C%20-46.599998474121094\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%C2%ABinterface%C2%BB%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-75.0015640258789%2C%20-28.19999885559082\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3ELiftMovementStrategy%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-141.1320343017578%2C%2014.200000762939453\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-141.1320343017578%2C%2044.20000076293945\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BfindNextStop\(lift%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20pendingStops\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-153.1320343017578%20-5.799999237060547%20C-68.81063025032488%20-5.799999237060547%2C%2015.510773801108058%20-5.799999237060547%2C%20153.1320343017578%20-5.799999237060547%20M-153.1320343017578%20-5.799999237060547%20C-47.96500757251664%20-5.799999237060547%2C%2057.20201915672453%20-5.799999237060547%2C%20153.1320343017578%20-5.799999237060547%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-153.1320343017578%2018.200000762939453%20C-41.55265501876194%2018.200000762939453%2C%2070.02672426423393%2018.200000762939453%2C%20153.1320343017578%2018.200000762939453%20M-153.1320343017578%2018.200000762939453%20C-83.55727183553287%2018.200000762939453%2C%20-13.982509369307934%2018.200000762939453%2C%20153.1320343017578%2018.200000762939453%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-LiftOperationStore-45%22%20transform%3D%22translate\(791.9089330037433%2C%20527.4000091552734\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-175.841552734375%20-78.80000305175781%20L175.841552734375%20-78.80000305175781%20L175.841552734375%2078.80000305175781%20L-175.841552734375%2078.80000305175781%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-175.841552734375%20-78.80000305175781%20C-53.79115407598819%20-78.80000305175781%2C%2068.25924458239862%20-78.80000305175781%2C%20175.841552734375%20-78.80000305175781%20M-175.841552734375%20-78.80000305175781%20C-80.45053900003799%20-78.80000305175781%2C%2014.940474734299016%20-78.80000305175781%2C%20175.841552734375%20-78.80000305175781%20M175.841552734375%20-78.80000305175781%20C175.841552734375%20-25.814787071408837%2C%20175.841552734375%2027.17042890894014%2C%20175.841552734375%2078.80000305175781%20M175.841552734375%20-78.80000305175781%20C175.841552734375%20-36.80050608090027%2C%20175.841552734375%205.1989908899572725%2C%20175.841552734375%2078.80000305175781%20M175.841552734375%2078.80000305175781%20C96.86276307989019%2078.80000305175781%2C%2017.883973425405372%2078.80000305175781%2C%20-175.841552734375%2078.80000305175781%20M175.841552734375%2078.80000305175781%20C64.43127400326529%2078.80000305175781%2C%20-46.979004727844426%2078.80000305175781%2C%20-175.841552734375%2078.80000305175781%20M-175.841552734375%2078.80000305175781%20C-175.841552734375%2021.055011016200964%2C%20-175.841552734375%20-36.68998101935588%2C%20-175.841552734375%20-78.80000305175781%20M-175.841552734375%2078.80000305175781%20C-175.841552734375%2044.884123244595855%2C%20-175.841552734375%2010.968243437433898%2C%20-175.841552734375%20-78.80000305175781%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(0%2C%20-58.80000305175781\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-61.80810546875%2C%20-58.80000305175781\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3ELiftOperationStore%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-163.841552734375%2C%20-16.40000343322754\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-163.841552734375%2C%2013.599996566772461\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BaddStop\(liftId%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20floor%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20journeyDirection\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C12.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BremoveStop\(liftId%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20floor%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20journeyDirection\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C33.59999942779541\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BgetPendingStops\(liftId\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-175.841552734375%20-36.40000343322754%20C-103.49560697307444%20-36.40000343322754%2C%20-31.14966121177389%20-36.40000343322754%2C%20175.841552734375%20-36.40000343322754%20M-175.841552734375%20-36.40000343322754%20C-72.53874538525534%20-36.40000343322754%2C%2030.76406196386432%20-36.40000343322754%2C%20175.841552734375%20-36.40000343322754%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-175.841552734375%20-12.400003433227539%20C-91.36181057167079%20-12.400003433227539%2C%20-6.882068408966575%20-12.400003433227539%2C%20175.841552734375%20-12.400003433227539%20M-175.841552734375%20-12.400003433227539%20C-82.3214703753091%20-12.400003433227539%2C%2011.198611983756791%20-12.400003433227539%2C%20175.841552734375%20-12.400003433227539%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22node%20default%22%20id%3D%22classId-HallRequestRegistry-46%22%20transform%3D%22translate\(1126.2508366902666%2C%20538.1000061035156\)%22%3E%3Cg%20class%3D%22basic%20label-container%22%3E%3Cpath%20d%3D%22M-118.50035095214844%20-89.5%20L118.50035095214844%20-89.5%20L118.50035095214844%2089.5%20L-118.50035095214844%2089.5%22%20stroke%3D%22none%22%20stroke-width%3D%220%22%20fill%3D%22rgb\(9%2C%2023%2C%2044\)%22%20style%3D%22%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M-118.50035095214844%20-89.5%20C-37.84180163110686%20-89.5%2C%2042.816747689934715%20-89.5%2C%20118.50035095214844%20-89.5%20M-118.50035095214844%20-89.5%20C-66.48017058600178%20-89.5%2C%20-14.4599902198551%20-89.5%2C%20118.50035095214844%20-89.5%20M118.50035095214844%20-89.5%20C118.50035095214844%20-19.801024401689318%2C%20118.50035095214844%2049.897951196621364%2C%20118.50035095214844%2089.5%20M118.50035095214844%20-89.5%20C118.50035095214844%20-31.06799704414287%2C%20118.50035095214844%2027.36400591171426%2C%20118.50035095214844%2089.5%20M118.50035095214844%2089.5%20C24.197012896446864%2089.5%2C%20-70.10632515925471%2089.5%2C%20-118.50035095214844%2089.5%20M118.50035095214844%2089.5%20C24.280017626600426%2089.5%2C%20-69.94031569894759%2089.5%2C%20-118.50035095214844%2089.5%20M-118.50035095214844%2089.5%20C-118.50035095214844%2043.90814585462567%2C%20-118.50035095214844%20-1.6837082907486547%2C%20-118.50035095214844%20-89.5%20M-118.50035095214844%2089.5%20C-118.50035095214844%2020.307786123867075%2C%20-118.50035095214844%20-48.88442775226585%2C%20-118.50035095214844%20-89.5%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22annotation-group%20text%22%20transform%3D%22translate\(0%2C%20-69.5\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22label-group%20text%22%20transform%3D%22translate\(-67.2256851196289%2C%20-69.5\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22font-weight%3A%20bolder%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%20font-weight%3D%22%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22%22%3EHallRequestRegistry%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22members-group%20text%22%20transform%3D%22translate\(-106.50035095214844%2C%20-27.100000381469727\)%22%3E%3C%2Fg%3E%3Cg%20class%3D%22methods-group%20text%22%20transform%3D%22translate\(-106.50035095214844%2C%202.8999996185302734\)%22%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C-9.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2Bfind\(floor%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20direction\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C12.199999809265137\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2Bassign\(request%2C%3C%2Ftspan%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%20liftId\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C33.59999942779541\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BmarkServed\(request\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22label%22%20style%3D%22%22%20transform%3D%22translate\(0%2C54.999999046325684\)%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3Ctext%20y%3D%22-10.1%22%20style%3D%22%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3Ctspan%20font-style%3D%22normal%22%20class%3D%22text-inner-tspan%22%20font-weight%3D%22normal%22%3E%2BmarkPending\(request\)%3C%2Ftspan%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-118.50035095214844%20-47.10000038146973%20C-24.529912834422475%20-47.10000038146973%2C%2069.44052528330349%20-47.10000038146973%2C%20118.50035095214844%20-47.10000038146973%20M-118.50035095214844%20-47.10000038146973%20C-60.924182071716515%20-47.10000038146973%2C%20-3.3480131912845934%20-47.10000038146973%2C%20118.50035095214844%20-47.10000038146973%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22divider%22%20style%3D%22%22%3E%3Cpath%20d%3D%22M-118.50035095214844%20-23.100000381469727%20C-57.350136093288455%20-23.100000381469727%2C%203.800078765571527%20-23.100000381469727%2C%20118.50035095214844%20-23.100000381469727%20M-118.50035095214844%20-23.100000381469727%20C-48.42409917480079%20-23.100000381469727%2C%2021.652152602546863%20-23.100000381469727%2C%20118.50035095214844%20-23.100000381469727%22%20stroke%3D%22rgb\(31%2C%2078%2C%20148\)%22%20stroke-width%3D%221.3%22%20fill%3D%22none%22%20stroke-dasharray%3D%220%200%22%20style%3D%22%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edges%20edgePaths%22%3E%3Cpath%20d%3D%22M1359.6265538533526%2C148.20001220703125L1359.6265538533526%2C214.3000030517578%22%20id%3D%22id_ElevatorApp_LiftManagementService_2%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_ElevatorApp_LiftManagementService_2%22%20data-points%3D%22W3sieCI6MTM1OS42MjY1NTM4NTMzNTI2LCJ5IjoxNDguMjAwMDEyMjA3MDMxMjV9LHsieCI6MTM1OS42MjY1NTM4NTMzNTI2LCJ5IjoyMjAuMzAwMDAzMDUxNzU3OH1d%22%20marker-end%3D%22url\(%23mermaid-_r_44t__class-dependencyEnd\)%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M1274.4912287394204%2C148.20001220703125L1274.4912287394204%2C161.4170561817366Q1274.4912287394204%2C163.20001220703125%201273.4054423017935%2C164.61422576940436L1273.4054423017935%2C164.61422576940436Q1272.3196558641666%2C166.02843933177743%201270.9054423017935%2C167.11422576940433L1270.9054423017935%2C167.11422576940436Q1269.4912287394204%2C168.20001220703125%201267.7082727141258%2C168.20001220703125L673.7459534618179%2C168.20001220703125Q671.9629974365232%2C168.20001220703125%20670.5487838741501%2C169.28579864465814L670.5487838741501%2C169.28579864465817Q669.134570311777%2C170.37158508228507%20668.0487838741501%2C171.78579864465814L668.0487838741501%2C171.78579864465814Q666.9629974365232%2C173.20001220703125%20666.9629974365232%2C174.9829682323259L666.9629974365232%2C182.20001220703125%22%20id%3D%22id_ElevatorApp_LiftService_3%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_ElevatorApp_LiftService_3%22%20data-points%3D%22W3sieCI6MTI3NC40OTEyMjg3Mzk0MjA0LCJ5IjoxNDguMjAwMDEyMjA3MDMxMjV9LHsieCI6MTI3NC40OTEyMjg3Mzk0MjA0LCJ5IjoxNjguMjAwMDEyMjA3MDMxMjV9LHsieCI6NjY2Ljk2Mjk5NzQzNjUyMzIsInkiOjE2OC4yMDAwMTIyMDcwMzEyNX0seyJ4Ijo2NjYuOTYyOTk3NDM2NTIzMiwieSI6MTg4LjIwMDAxMjIwNzAzMTI1fV0%3D%22%20marker-end%3D%22url\(%23mermaid-_r_44t__class-dependencyEnd\)%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M1434.17965443929%2C377.90000915527344L1434.17965443929%2C442.6000061035156%22%20id%3D%22id_LiftManagementService_RequestLiftStrategy_4%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_LiftManagementService_RequestLiftStrategy_4%22%20data-points%3D%22W3sieCI6MTQzNC4xNzk2NTQ0MzkyOSwieSI6Mzc3LjkwMDAwOTE1NTI3MzQ0fSx7IngiOjE0MzQuMTc5NjU0NDM5MjksInkiOjQ0OC42MDAwMDYxMDM1MTU2fV0%3D%22%20marker-end%3D%22url\(%23mermaid-_r_44t__class-dependencyEnd\)%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M1359.6265538533526%2C377.90000915527344L1359.6265538533526%2C421.817050078221Q1359.6265538533526%2C423.6000061035156%201358.5407674157257%2C425.01421966588873L1358.5407674157257%2C425.01421966588873Q1357.4549809780988%2C426.42843322826184%201356.0407674157257%2C427.51421966588873L1356.0407674157257%2C427.51421966588873Q1354.6265538533526%2C428.6000061035156%201352.843597828058%2C428.6000061035156L1133.0337927155613%2C428.6000061035156Q1131.2508366902666%2C428.6000061035156%201129.8366231278935%2C429.6857925411425L1129.8366231278935%2C429.6857925411425Q1128.4224095655204%2C430.7715789787694%201127.3366231278935%2C432.1857925411425L1127.3366231278935%2C432.1857925411425Q1126.2508366902666%2C433.6000061035156%201126.2508366902666%2C435.3829621288103L1126.2508366902666%2C442.6000061035156%22%20id%3D%22id_LiftManagementService_HallRequestRegistry_5%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_LiftManagementService_HallRequestRegistry_5%22%20data-points%3D%22W3sieCI6MTM1OS42MjY1NTM4NTMzNTI2LCJ5IjozNzcuOTAwMDA5MTU1MjczNDR9LHsieCI6MTM1OS42MjY1NTM4NTMzNTI2LCJ5Ijo0MjguNjAwMDA2MTAzNTE1Nn0seyJ4IjoxMTI2LjI1MDgzNjY5MDI2NjYsInkiOjQyOC42MDAwMDYxMDM1MTU2fSx7IngiOjExMjYuMjUwODM2NjkwMjY2NiwieSI6NDQ4LjYwMDAwNjEwMzUxNTZ9XQ%3D%3D%22%20marker-end%3D%22url\(%23mermaid-_r_44t__class-dependencyEnd\)%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M1285.073453267415%2C377.90000915527344L1285.073453267415%2C401.817050078221Q1285.073453267415%2C403.6000061035156%201283.9876668297882%2C405.01421966588873L1283.9876668297882%2C405.01421966588873Q1282.9018803921613%2C406.42843322826184%201281.4876668297882%2C407.51421966588873L1281.4876668297882%2C407.51421966588873Q1280.073453267415%2C408.6000061035156%201278.2904972421204%2C408.6000061035156L857.3057399404963%2C408.6000061035156Q855.5227839152017%2C408.6000061035156%20854.1085703528286%2C409.6857925411425L854.1085703528286%2C409.6857925411425Q852.6943567904555%2C410.7715789787694%20851.6085703528286%2C412.1857925411425L851.6085703528286%2C412.1857925411425Q850.5227839152017%2C413.6000061035156%20850.5227839152017%2C415.3829621288103L850.5227839152017%2C442.6000061035156%22%20id%3D%22id_LiftManagementService_LiftOperationStore_6%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_LiftManagementService_LiftOperationStore_6%22%20data-points%3D%22W3sieCI6MTI4NS4wNzM0NTMyNjc0MTUsInkiOjM3Ny45MDAwMDkxNTUyNzM0NH0seyJ4IjoxMjg1LjA3MzQ1MzI2NzQxNSwieSI6NDA4LjYwMDAwNjEwMzUxNTZ9LHsieCI6ODUwLjUyMjc4MzkxNTIwMTcsInkiOjQwOC42MDAwMDYxMDM1MTU2fSx7IngiOjg1MC41MjI3ODM5MTUyMDE3LCJ5Ijo0NDguNjAwMDA2MTAzNTE1Nn1d%22%20marker-end%3D%22url\(%23mermaid-_r_44t__class-dependencyEnd\)%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M666.9629974365232%2C388.6000061035156L666.9629974365232%2C421.817050078221Q666.9629974365232%2C423.6000061035156%20665.8772109988963%2C425.01421966588873L665.8772109988963%2C425.01421966588873Q664.7914245612694%2C426.42843322826184%20663.3772109988963%2C427.51421966588873L663.3772109988963%2C427.51421966588873Q661.9629974365232%2C428.6000061035156%20660.1800414112286%2C428.6000061035156L429.71830199290514%2C428.6000061035156Q427.9353459676105%2C428.6000061035156%20426.5211324052374%2C429.6857925411425L426.5211324052374%2C429.6857925411425Q425.1069188428643%2C430.7715789787694%20424.0211324052374%2C432.1857925411425L424.0211324052374%2C432.1857925411425Q422.9353459676105%2C433.6000061035156%20422.9353459676105%2C435.3829621288103L422.9353459676105%2C442.6000061035156%22%20id%3D%22id_LiftService_LiftMovementStrategy_7%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_LiftService_LiftMovementStrategy_7%22%20data-points%3D%22W3sieCI6NjY2Ljk2Mjk5NzQzNjUyMzIsInkiOjM4OC42MDAwMDYxMDM1MTU2fSx7IngiOjY2Ni45NjI5OTc0MzY1MjMyLCJ5Ijo0MjguNjAwMDA2MTAzNTE1Nn0seyJ4Ijo0MjIuOTM1MzQ1OTY3NjEwNSwieSI6NDI4LjYwMDAwNjEwMzUxNTZ9LHsieCI6NDIyLjkzNTM0NTk2NzYxMDUsInkiOjQ0OC42MDAwMDYxMDM1MTU2fV0%3D%22%20marker-end%3D%22url\(%23mermaid-_r_44t__class-dependencyEnd\)%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M733.2950820922849%2C388.6000061035156L733.2950820922849%2C442.6000061035156%22%20id%3D%22id_LiftService_LiftOperationStore_8%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_LiftService_LiftOperationStore_8%22%20data-points%3D%22W3sieCI6NzMzLjI5NTA4MjA5MjI4NDksInkiOjM4OC42MDAwMDYxMDM1MTU2fSx7IngiOjczMy4yOTUwODIwOTIyODQ5LCJ5Ijo0NDguNjAwMDA2MTAzNTE1Nn1d%22%20marker-end%3D%22url\(%23mermaid-_r_44t__class-dependencyEnd\)%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M600.6309127807615%2C388.6000061035156L600.6309127807615%2C401.817050078221Q600.6309127807615%2C403.6000061035156%20599.5451263431346%2C405.01421966588873L599.5451263431346%2C405.01421966588873Q598.4593399055077%2C406.42843322826184%20597.0451263431346%2C407.51421966588873L597.0451263431346%2C407.51421966588873Q595.6309127807615%2C408.6000061035156%20593.8479567554668%2C408.6000061035156L175.054656566473%2C408.6000061035156Q173.27170054117835%2C408.6000061035156%20171.85748697880524%2C409.6857925411425L171.85748697880524%2C409.6857925411425Q170.44327341643216%2C410.7715789787694%20169.35748697880527%2C412.1857925411425L169.35748697880524%2C412.1857925411425Q168.27170054117835%2C413.6000061035156%20168.27170054117835%2C415.3829621288103L168.27170054117835%2C442.6000061035156%22%20id%3D%22id_LiftService_Lift_9%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_LiftService_Lift_9%22%20data-points%3D%22W3sieCI6NjAwLjYzMDkxMjc4MDc2MTUsInkiOjM4OC42MDAwMDYxMDM1MTU2fSx7IngiOjYwMC42MzA5MTI3ODA3NjE1LCJ5Ijo0MDguNjAwMDA2MTAzNTE1Nn0seyJ4IjoxNjguMjcxNzAwNTQxMTc4MzUsInkiOjQwOC42MDAwMDYxMDM1MTU2fSx7IngiOjE2OC4yNzE3MDA1NDExNzgzNSwieSI6NDQ4LjYwMDAwNjEwMzUxNTZ9XQ%3D%3D%22%20marker-end%3D%22url\(%23mermaid-_r_44t__class-dependencyEnd\)%22%3E%3C%2Fpath%3E%3Cpath%20d%3D%22M106.7400894165039%2C405.8500061035156L106.7400894165039%2C448.6000061035156%22%20id%3D%22id_Building_Lift_1%22%20class%3D%22edge-thickness-normal%20edge-pattern-solid%20relation%22%20style%3D%22%3B%3B%3B%22%20data-edge%3D%22true%22%20data-et%3D%22edge%22%20data-id%3D%22id_Building_Lift_1%22%20data-points%3D%22W3sieCI6MTA2Ljc0MDA4OTQxNjUwMzksInkiOjM4OC42MDAwMDYxMDM1MTU2fSx7IngiOjEwNi43NDAwODk0MTY1MDM5LCJ5Ijo0NDguNjAwMDA2MTAzNTE1Nn1d%22%20marker-start%3D%22url\(%23mermaid-_r_44t__class-aggregationStart\)%22%3E%3C%2Fpath%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabels%22%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%3E%3Crect%20class%3D%22background%22%20style%3D%22stroke%3A%20none%22%3E%3C%2Frect%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_Building_Lift_1%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_ElevatorApp_LiftManagementService_2%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_ElevatorApp_LiftService_3%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_LiftManagementService_RequestLiftStrategy_4%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_LiftManagementService_HallRequestRegistry_5%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_LiftManagementService_LiftOperationStore_6%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_LiftService_LiftMovementStrategy_7%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_LiftService_LiftOperationStore_8%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeLabel%22%3E%3Cg%20class%3D%22label%22%20data-id%3D%22id_LiftService_Lift_9%22%20transform%3D%22translate\(0%2C%200\)%22%3E%3Ctext%20y%3D%22-10.1%22%3E%3Ctspan%20class%3D%22text-outer-tspan%22%20x%3D%220%22%20y%3D%22-0.1em%22%20dy%3D%221.1em%22%3E%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeTerminals%22%20transform%3D%22translate\(91.74008970825196%2C%20406.10000830182753\)%22%3E%3Cg%20class%3D%22inner%22%20transform%3D%22translate\(-2.96875%2C%20-7.200000286102295\)%22%3E%3Ctext%20style%3D%22%22%3E%3Ctspan%20xml%3Aspace%3D%22preserve%22%20dy%3D%221em%22%20x%3D%220%22%20class%3D%22row%22%3E1%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3Cg%20class%3D%22edgeTerminals%22%20transform%3D%22translate\(116.74008970825196%2C%20426.1000083018276\)%22%3E%3Cg%20class%3D%22inner%22%20transform%3D%22translate\(-2.4000000953674316%2C%20-7.200000286102295\)%22%3E%3C%2Fg%3E%3Ctext%20style%3D%22%22%3E%3Ctspan%20xml%3Aspace%3D%22preserve%22%20dy%3D%221em%22%20x%3D%220%22%20class%3D%22row%22%3E*%3C%2Ftspan%3E%3C%2Ftext%3E%3C%2Fg%3E%3C%2Fg%3E%3C%2Fsvg%3E)

## 13. Your interview implementation order

Build the simplest working system first:

1. Define `Building`, `Floor`, `Lift`, direction/status enums, and `PendingStops`.

2. Implement `LiftOperationStore` with the two ordered sets.

3. Implement `LiftService` with internal button presses and a worker that moves floor by floor.

4. Implement `LiftManagementService` and a simple `RequestLiftStrategy`.

5. Add the hall-request registry to prevent duplicate assignments and track completion.

6. Add per-lift synchronization, condition variables, and safety checks.

7. Extend scheduling, capacity handling, and state behavior only as the interviewer asks.

The core design to remember: The management service assigns pickups; the operation store holds each lift's ordered stops; the movement strategy chooses the next stop; and each lift's worker executes the journey safely and independently.
