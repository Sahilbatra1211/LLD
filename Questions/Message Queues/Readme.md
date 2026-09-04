# Message Queues — Notes

First: **what a queue is for**. Brokers (Kafka, RabbitMQ, SQS) come later.

---

## 1. Why do we need a Message Queue?

```text
Producer                    Queue                    Consumer
   |                          |                          |
   |  put work                |                          |
   |------------------------->|                          |
   |  (does not wait)         |     take when ready      |
   |                          |<-------------------------|
```

| Need | What the queue gives you |
|---|---|
| **Asynchronous processing** | Producer does not wait for the consumer to finish. |
| **Decoupling** | Producer and consumer need not be up at the same time. |
| **Buffering** | Absorbs spikes when producers are faster than consumers. |
| **Reliability** | Message can sit in the queue if the consumer is down. |
| **Retry** | Failed processing can be tried again. |
| **Failure recovery** | After a crash, messages can be redelivered. |
| **Load distribution** | Several consumers can share the work. |
| **Backpressure** | Stops consumers from being flooded. |
| **Ordering** | Can guarantee order when you need it. |
| **Acknowledgement (ACK)** | You know whether the message was processed. |
| **Dead Letter Queue (DLQ)** | Messages that keep failing are isolated. |
| **Scalability** | Workers scale independently of producers. |
| **Durability** | Messages can survive app/consumer crashes (depends on the broker). |

### One-line mental model

> A message queue is not just for async calls; it provides **reliable, decoupled, buffered delivery of work** between producers and consumers.

If you only needed “call this later,” a thread pool might be enough. The queue is for **delivery guarantees** (retry, ACK, DLQ, survive downtime) and **independent scaling**.

---

## 3. Queue

A queue is a **buffer of messages**:

```text
Producer
   ↓
┌─────────────────────────────┐
│ M1 │ M2 │ M3 │ M4 │ M5     │
└─────────────────────────────┘
   ↓
Consumer
```

Producer and consumer **do not have to run at the same speed**.

Example:

```text
Producer: 10,000 messages/sec
Consumer:  2,000 messages/sec
```

The queue **absorbs the difference**:

```text
Producer
   ↓
████████████████████ Queue
                         ↓
                      Consumer
```

That is **buffering**.

The producer can keep publishing. Extra messages sit in the queue until the consumer catches up (or the queue fills — then you get **backpressure** / dropped publish, depending on the broker).

---

## 4. Why not just use a database?

You could do:

```sql
INSERT INTO jobs (...)
```

and workers could **poll** that table.

A message queue is built for **delivery and consumption**, not as a general store you poll.

They typically give you:

- Efficient **sequential** consumption
- **Acknowledgement**
- **Redelivery**
- **Consumer coordination** (who gets which message)
- **Ordering**
- **Backpressure**
- **Retries**
- **Dead-letter** handling

A database **can** be used to fake some of this (status columns, `SELECT … FOR UPDATE SKIP LOCKED`, cron pollers). You then own locking, double-processing, and backoff yourself.

A queue gives you that **abstraction directly**.

> DB = source of truth for **entities**. Queue = **moving work** between services with consume semantics.

---

## 5. Push vs Pull

### Pull

The consumer asks: *“Do you have any messages for me?”*

```text
Consumer ──> Queue: give me message
Consumer <── Queue: M1

Consumer ──> Queue: give me message
Consumer <── Queue: M2
```

The **consumer** controls the rate.

### Push

The queue says: *“Here’s a message.”*

```text
Queue ──> Consumer: M1
Queue ──> Consumer: M2
Queue ──> Consumer: M3
```

The **broker** controls delivery.

### Why it matters

Consumer can do **100 messages/sec**. Queue pushes **10,000/sec**:

```text
Queue
 ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
Consumer 💀
```

You need **flow control / backpressure**.

**Pull** naturally lets the consumer set the pace. That matters when we design our own queue.

(Real brokers mix both: e.g. Kafka is pull; some systems push with prefetch/credits so they don’t flood the worker.)

---

## 6. Message Queue vs Pub/Sub

### Normal queue — work distribution

Producer puts messages on a **queue**. Several consumers can read from it.

```text
Producer
   |
   v
+-----------+
|   Queue   |
| M1 M2 M3  |
+-----------+
   |     |
   v     v
  C1     C2
```

Consumers are **competing consumers**:

```text
M1 → C1
M2 → C2
M3 → C1
```

> A given message is normally consumed by **only one** consumer.

Ten workers on the same queue **share the work**; they do not each get a copy of `M1`.

**Use:** *“Here is work. I want **one** worker to do it.”*

```text
Order → Queue → Worker 1 / Worker 2 / Worker 3
```

---

### Pub/Sub — event distribution

Subscribers independently subscribe to a **topic**. They do **not** compete for the same copy.

```text
                Producer
                   |
                   v
                Topic
                   |
        +----------+----------+
        |          |          |
        v          v          v
   Subscriber A Subscriber B Subscriber C
```

Publish `ORDER_CREATED` → every interested subscriber can get it:

```text
ORDER_CREATED
      +----> Subscriber A
      +----> Subscriber B
      +----> Subscriber C
```

> The **same** event can be processed independently by **many** subscribers.

---

### What is a topic?

A **named category / stream of events**, e.g. `ORDER_EVENTS`, `PAYMENT_EVENTS`, `USER_EVENTS`.

```text
publish("ORDER_EVENTS", orderCreatedEvent)
subscribe("ORDER_EVENTS")
```

One event, many reasons:

```text
                 ORDER_EVENTS
                      |
                ORDER_CREATED
          +-----------+-----------+
          |           |           |
          v           v           v
        Email      Analytics    Inventory
```

- Email → confirmation  
- Analytics → stats  
- Inventory → stock  

That is why pub/sub exists: **one event, many independent consumers.**

---

### Where queues fit inside Pub/Sub

In a real broker, **each subscription often has its own queue**.

```text
                    Topic
                 ORDER_EVENTS
                      |
          +-----------+-----------+
          |           |           |
          v           v           v
     Email Sub   Analytics Sub  Inventory Sub
          |           |           |
          v           v           v
     Email Queue  Analytics Q   Inventory Q
```

Publish `ORDER_CREATED` → **a copy is buffered per subscription**:

```text
Email Queue:       ORDER_CREATED
Analytics Queue:   ORDER_CREATED
Inventory Queue:   ORDER_CREATED
```

Email being slow does **not** block Analytics. Isolation is per subscription queue.

---

### Workers / threads (competing *inside* a subscription)

A subscription can have **many worker threads** on **its** queue.

```text
Email Subscription
        |
    Email Queue
     /   |   \
    T1   T2   T3
```

`T1`, `T2`, `T3` are **competing consumers of the Email queue only**:

```text
M1 → T1
M2 → T2
M3 → T3
M4 → T1
```

Across subscriptions, the **same** `ORDER_CREATED` still went to Email **and** Analytics. Inside Email, **one** thread processes each copy.

Putting threads on different topics is a **scaling choice**, not a rule of pub/sub.

```text
Topic (fan-out)     →  one copy per subscription
Subscription queue  →  competing workers share that copy
```

---

### Queue vs Pub/Sub

| | Normal queue | Pub/Sub |
|---|---|---|
| Purpose | Distribute **work** | Distribute **events** |
| Multiple consumers? | Yes | Yes |
| Same message to many consumers? | Normally **no** | **Yes** |
| Compete? | **Yes** | **No** — independent subscribers |
| Concept | Queue | Topic |
| Example | Process this image (one worker) | Image uploaded — email + analytics + thumbnail |

### One-line mental model

> **Queue →** “Someone needs to do this work.”  
> **Pub/Sub →** “Something happened; whoever cares can react.”

---

### Don’t mix with push vs pull

**Push/pull:** who **starts** delivery?  
- Pull: consumer asks.  
- Push: the system sends.

**Queue vs pub/sub:** who **receives** the message?  
- Queue: one competing consumer (usually).  
- Pub/sub: many independent subscribers.

You can mix:

```text
Queue + Pull
Queue + Push
Pub/Sub + Pull
Pub/Sub + Push
```

---

### Delivery semantics vs the name “queue”

If a push-based “message queue” did this:

```text
Message Queue
     |---- push M1 → Consumer A
     |---- push M1 → Consumer B
     |---- push M1 → Consumer C
```

that is **pub/sub-style broadcast**, not a competing-consumer queue (`M1` to only one worker).

The **name** `MessageQueue` does not decide the model. **Who gets a copy of M1** does.

---

## 7. Consumer Groups

Same idea as **competing workers on one subscription queue**, with a name Kafka uses.

Order queue + three workers — they **share** the work:

```text
M1 → A
M2 → B
M3 → C
M4 → A
M5 → B
```

Those are competing consumers.

Kafka calls a set of such workers a **consumer group**.

```text
Topic: orders

Consumer Group: EmailWorkers
    Worker A
    Worker B
    Worker C
```

**Within a group:** one message → **one** worker.

**Another group** can still get the **same** event independently:

```text
orders
  │
  ├── Email Group
  │      ├── A
  │      └── B
  │
  └── Analytics Group
         ├── C
         └── D
```

Email **and** Analytics both see the event. `A`/`B` share Email’s load; `C`/`D` share Analytics’ load.

That is **pub/sub across groups** + **queue inside a group** — a core Kafka idea.

> **Group** ≈ one subscription. **Workers in the group** ≈ competing consumers on that subscription’s queue.

---

## 8. ACK — extremely important

The consumer is processing:

```text
Queue → Consumer
       ↓
   processing
```

The queue must know: **did this message succeed?**

That’s **acknowledgement**.

```text
Queue → M1 → Consumer
             ↓
          process()
             ↓
            ACK
             ↓
Queue knows M1 succeeded
```

Without ACK, if the consumer **crashes**:

```text
Queue → M1 → Consumer
             ↓
           crash 💥
```

If the queue had already **deleted** `M1`:

```text
M1 → LOST ❌
```

So most queues do **not** treat a message as done until the consumer **ACKs**. Until then it can be **redelivered** (at-least-once). ACK too late or never → duplicates; ACK before the work is durable → loss. That’s the tradeoff behind “at-least-once vs at-most-once.”

---

## 9. The famous failure case

Important for upcoming LLD.

```text
Queue → Consumer
          ↓
      process M1
          ↓
      DB updated ✅
          ↓
      Consumer crashes 💥
          ↓
        ACK never sent
```

The queue still thinks **M1 was not processed**, so it delivers **again**:

```text
Queue → Consumer
          ↓
      process M1 again
```

That’s **duplicate processing**. Work (DB write) succeeded; **ACK did not**.

This is why **idempotency** matters: processing `M1` twice must be safe (same row, same email, same payment intent — not a second charge).

**Same idea as Notification System:** retry / fallback / a second provider can **send twice**. `ProviderDeliveryService` failing over is like redelivery. Channels and providers should treat “same notification id” as **at-least-once**, so the user doesn’t get two payment SMS unless you accept that, or you **dedupe** on id.

> At-least-once + crash between side effect and ACK → **duplicates**. Design consumers (and notification send) to be **idempotent**.

---

## 10. Delivery guarantees

Three terms to know.

| Guarantee | Meaning | Tradeoff |
|---|---|---|
| **At-most-once** | Processed **0 or 1** time | Can be **lost**. No duplicate processing. |
| **At-least-once** | Processed **1 or more** times | Shouldn’t lose the message; **duplicates** possible. Very common. Consumers should be **idempotent**. |
| **Exactly-once** | Processed **exactly once** | Sounds ideal. Across distributed systems and **external side effects** (SMS, card charge), it is **hard** to truly guarantee. |

Same realization as **notification providers**: retry/fallback is at-least-once; “exactly one SMS” needs extra design (idempotency keys, provider-side dedupe), not just the queue slogan.

In practice, systems often aim for:

> **At-least-once delivery + idempotent processing**

---

## 11. Retry

Consumer calls Payment Service → **timeout**. You usually **don’t want to drop** the message.

```text
M1
 ↓
Consumer
 ↓
FAIL
 ↓
Retry
 ↓
Consumer
 ↓
SUCCESS
```

If it **never** succeeds:

```text
M1 → fail
M1 → fail
M1 → fail
M1 → fail
M1 → fail
```

You **cannot retry forever** (poison message, bad payload, downstream permanently down). Cap attempts; after that, isolate the message (see **DLQ** below). Same as notification: retry/fallback, then stop.

---

## 12. Dead Letter Queue (DLQ)

After enough failures:

```text
Main Queue
    ↓
Consumer
    ↓
fail → retry → fail → retry → fail
    ↓
DLQ
```

**DLQ** = Dead Letter Queue. The message is **moved aside** for later investigation or reprocessing — not retried forever on the main queue.

Example:

```text
orders
   ↓
Order Worker
   ↓
5 failures
   ↓
orders-dlq
```

That stops one **poison / broken** message from looping forever and blocking (or drowning) the rest of the workload.

---

## 14. Ordering

Producer sends:

```text
M1
M2
M3
```

Do we guarantee **M1 → M2 → M3**? **Not necessarily.**

With multiple consumers:

```text
M1 → Consumer A
M2 → Consumer B
M3 → Consumer C
```

`M3` might **finish first**. Parallel workers + different runtimes break global order even if they were dequeued in order.

Ask explicitly:

> Does this system **require** ordering?

If yes:

> **Globally**, **per queue**, or **per key / user / entity**?

Example: payments on **one account** might need:

```text
Payment1 → Payment2 → Payment3
```

Payments on **different accounts** can run **in parallel**.

That leads to **partitioning** (later): same key → same partition → order for that key; different keys → different partitions → more throughput.

---

## Message Ordering — more detail

### Simple FIFO

One queue, **one** consumer, sequential processing:

```text
M1 → M2 → M3
        ↓
M1 processed, then M2, then M3
```

Order is easy.

### Where it breaks

Several workers:

```text
Queue: M1 M2 M3

M1 → Worker 1
M2 → Worker 2
M3 → Worker 3
```

The queue may still **hand out** FIFO (`M1`, then `M2`, then `M3`). **Finish** order can be:

```text
M2 → M3 → M1
```

Workers take different amounts of time.

> **Delivery order ≠ processing completion order.**

### Workers belong to the consumer / service

Workers = threads, processes, or instances:

```text
PaymentService
    ├── Worker 1
    ├── Worker 2
    └── Worker 3
```

The **service** scales by consuming **concurrently**.

### Global vs key-based ordering

```text
M1 → Order 101 → CREATED
M2 → Order 202 → CREATED
M3 → Order 101 → PAID
M4 → Order 202 → PAID
```

Need:

```text
Order 101: CREATED → PAID
Order 202: CREATED → PAID
```

**Not** necessarily a global order between 101 and 202. **Key** = `OrderId`.

> **Key-based ordering** = order among messages for the **same entity**. Parallelism across different keys.

### Topic vs key

| | Question |
|---|---|
| **Topic** | What **category** of events? (`ORDER_EVENTS`, `PAYMENT_EVENTS`) |
| **Key** | Which **entity**? (`OrderId` on `ORDER_EVENTS`) |

One topic can hold events for millions of entities.

### Mental model

> **Topic** → what kind of event?  
> **Key** → which entity?  
> **Ordering** → in what sequence must messages for **that** entity be processed?

---

## 15. Backpressure

```text
Producer = 100,000 msg/sec
Consumer =  10,000 msg/sec
```

The queue **grows**: 10K → 20K → 30K → 40K → … → a huge **backlog**.

Levers (this is **backpressure** / dealing with overload):

- Limit **producers**
- Add **consumers**
- **Rate-limit** consumers (or prefetch)
- **Queue size limits**
- **Prioritization**
- **Load shedding** (drop / reject when full)

Without some of these, memory explodes or latency goes to infinity. Same idea as push flooding a slow worker (section 5).

---

## 16. Kafka vs RabbitMQ (concept only)

Don’t need implementation details yet.

### RabbitMQ-style

Centered on **queues and routing**:

```text
Producer
   ↓
Exchange
   ↓
Queue
   ↓
Consumer
```

Mental model: put on a queue → consume → **remove** (after ACK).

### Kafka-style

Centered on a **distributed append-only event log**:

```text
Producer
   ↓
Topic
   ↓
Partitions
   ↓
Consumers / Consumer Groups
```

Messages stay for a **retention** period. Consumers track an **offset** (position in the log):

```text
M1 M2 M3 M4 M5 M6
          ↑
       offset
```

A consumer can **continue from its offset** (or rewind, depending on config). That is **not** “take from queue and delete for everyone.” Other groups have their **own** offsets.

---

## Mental model before LLD

```text
                    MESSAGE SYSTEM
                          │
            ┌─────────────┴─────────────┐
            ↓                           ↓
         QUEUE                        PUB/SUB
            │                           │
     work distribution            event distribution
            │                           │
      competing consumers          many subscribers
            │
            ↓
       ACK / retry
            │
      ┌─────┴─────┐
      ↓           ↓
   success      failure
                  ↓
                retry
                  ↓
             repeated failure
                  ↓
                 DLQ
```

### Five questions whenever someone says “message queue”

1. **Who produces** the message?  
2. **Who consumes** it — and can there be **multiple** consumers?  
3. When is it **done**? → **ACK**  
4. What if processing **fails**? → **retry / DLQ**  
5. What **delivery guarantee**? → at-most-once / at-least-once / exactly-once  

Once those are clear, we can design an **in-memory Message Queue LLD**.













