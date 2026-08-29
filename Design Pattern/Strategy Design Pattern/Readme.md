# Strategy Design Pattern — Cricket Example

## Definition

> Encapsulate a family of interchangeable behaviors and compose them into a context, so the behavior can vary independently of the type that uses it.

In this example:

> Use **inheritance** for different types of batsmen, and **Strategy** for interchangeable shot behavior.

---

## 1. Problem

Suppose we are designing a cricket game.

A cricket player can be of different types:

- `Batsman`
- `AggressiveBatsman`
- `DefensiveBatsman`

At the same time, a batsman can play different types of shots:

- Drive
- Pull
- Cut
- Sweep

These are **two different dimensions of behavior**.

We don't want to create classes like:

```text
AggressiveBatsmanWhoPlaysDrive
AggressiveBatsmanWhoPlaysPull
AggressiveBatsmanWhoPlaysCut
DefensiveBatsmanWhoPlaysDrive
DefensiveBatsmanWhoPlaysPull
...
```

This would cause a **class explosion**.

Instead, we use:

- **Inheritance** for different types of batsmen
- **Strategy Pattern** for interchangeable shot behavior

---

## 2. Overall Design

```text
                    Batsman
                       ↑
              ┌────────┴────────┐
              ↓                 ↓
     AggressiveBatsman    DefensiveBatsman


                    Batsman
                       |
                       | HAS-A
                       ↓
                  ShotStrategy
                       ↑
              ┌────────┼────────┐
              ↓        ↓        ↓
           Drive      Pull      Cut
```

There are two separate relationships.

### IS-A

```text
AggressiveBatsman IS-A Batsman
DefensiveBatsman  IS-A Batsman
```

This is inheritance.

### HAS-A

```text
Batsman HAS-A ShotStrategy
```

This is composition.

---

## 3. Strategy Interface

We create a common interface for all shots:

```cpp
class ShotStrategy {
public:
    virtual void playShot() = 0;
    virtual ~ShotStrategy() = default;
};
```

This defines the contract:

> Every shot strategy must know how to `playShot()`.

It doesn't contain the implementation of a particular shot.

---

## 4. Concrete Shot Strategies

### Drive

```cpp
class Drive : public ShotStrategy {
public:
    void playShot() override {
        cout << "Playing a drive\n";
    }
};
```

### Pull

```cpp
class Pull : public ShotStrategy {
public:
    void playShot() override {
        cout << "Playing a pull shot\n";
    }
};
```

### Cut

```cpp
class Cut : public ShotStrategy {
public:
    void playShot() override {
        cout << "Playing a cut shot\n";
    }
};
```

So:

```text
              ShotStrategy
                   ↑
          ┌────────┼────────┐
          ↓        ↓        ↓
        Drive     Pull      Cut
```

---

## 5. Batsman — The Context

`Batsman` uses a `ShotStrategy`.

```cpp
class Batsman {
protected:
    ShotStrategy* shotStrategy;

public:
    Batsman(ShotStrategy* strategy)
        : shotStrategy(strategy) {}

    void playShot() {
        shotStrategy->playShot();
    }

    void setShotStrategy(ShotStrategy* strategy) {
        shotStrategy = strategy;
    }

    virtual ~Batsman() = default;
};
```

Notice:

```cpp
ShotStrategy* shotStrategy;
```

The `Batsman` doesn't know whether it is:

```text
Drive
Pull
Cut
```

It only knows:

```text
ShotStrategy
```

This is **polymorphism**.

---

## 6. Different Types of Batsmen

Now we can have different types of batsmen.

### Aggressive Batsman

```cpp
class AggressiveBatsman : public Batsman {
public:
    AggressiveBatsman(ShotStrategy* strategy)
        : Batsman(strategy) {}

    void play() {
        cout << "Playing aggressively\n";
        playShot();
    }
};
```

### Defensive Batsman

```cpp
class DefensiveBatsman : public Batsman {
public:
    DefensiveBatsman(ShotStrategy* strategy)
        : Batsman(strategy) {}

    void play() {
        cout << "Playing defensively\n";
        playShot();
    }
};
```

Now we have two independent concepts:

```text
Batsman Type                 Shot Type

AggressiveBatsman            Drive
DefensiveBatsman             Pull
                             Cut
```

---

## 7. Dependency Injection

The shot strategy is provided from outside.

```cpp
Drive drive;
Pull pull;

AggressiveBatsman player1(&drive);
DefensiveBatsman player2(&pull);
```

Here:

```text
player1
   |
   ↓
 Drive

player2
   |
   ↓
 Pull
```

The `Batsman` doesn't create the strategy itself.

The strategy is **injected** through the constructor.

---

## 8. Runtime Strategy Change

The strategy can also be changed.

```cpp
AggressiveBatsman player(&drive);

player.play();
```

Output:

```text
Playing aggressively
Playing a drive
```

Later:

```cpp
player.setShotStrategy(&pull);

player.play();
```

Output:

```text
Playing aggressively
Playing a pull shot
```

The player's type didn't change. `AggressiveBatsman` remained the same.

Only the **shot strategy** changed:

```text
Drive → Pull
```

---

## 9. Why This Design Is Better

Without Strategy, we could end up with:

```text
AggressiveDriveBatsman
AggressivePullBatsman
AggressiveCutBatsman

DefensiveDriveBatsman
DefensivePullBatsman
DefensiveCutBatsman
```

If we add:

```text
Sweep
Hook
ReverseSweep
```

the number of classes keeps increasing.

This happens because we are combining two independent dimensions:

```text
Batsman Type × Shot Type
```

Strategy separates those dimensions.

```text
Batsman Type
     ↓
Inheritance

Shot Type
     ↓
Strategy
```

---

## 10. Important Design Insight

This example demonstrates when **composition is better than inheritance**.

Use inheritance for the stable relationship:

```text
AggressiveBatsman IS-A Batsman
```

Use Strategy for behavior that needs to vary independently:

```text
Batsman HAS-A ShotStrategy
```

This prevents a large inheritance hierarchy.

---

## 11. Complete Structure

```text
                    Batsman
                   /       \
                  /         \
                 ↓           ↓
      AggressiveBatsman   DefensiveBatsman
                 |
                 | HAS-A
                 ↓
            ShotStrategy
                 ↑
        ┌────────┼────────┐
        ↓        ↓        ↓
      Drive     Pull      Cut
```

The inheritance hierarchy answers:

> **"What type of batsman is this?"**

The Strategy hierarchy answers:

> **"How does this batsman play a shot?"**

These are separate questions and therefore should be modeled separately.

---

## 12. Core Interview Explanation

> We use inheritance to model different types of batsmen, while Strategy Pattern is used to encapsulate interchangeable shot behaviors. `Batsman` composes a `ShotStrategy` through a common interface, and the strategy can be injected through the constructor and changed at runtime.

The important relationships are:

```text
IS-A  → inheritance
HAS-A → composition
```

And the key idea is:

```text
Different player types
        ↓
    Inheritance

Different shot behaviors
        ↓
     Strategy
```

### Strategy Pattern = Composition + Polymorphism + Interchangeable Behavior
