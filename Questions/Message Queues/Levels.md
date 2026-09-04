# Message Queue — Full Level Roadmap

Concept notes: [Readme.md](./Readme.md) · Combined LLD sketch: [LLD.md](./LLD.md)

**Folders on disk today**

| Folder | Matches roadmap |
|---|---|
| [Level 1](./Level%201/Readme.md) | Rows **1 + 2** (FIFO **and** mutex / `condition_variable`) |
| [Level 2](./Level%202/Readme.md) | Row **3** (ACK + in-flight) |
| [Level 3](./Level%203/Readme.md) | Row **4** (retry, backoff, DLQ) |
| [Level 4](./Level%204/Readme.md) | Rows **5 + 6** (multiple queues + topics / pub-sub) |

---

| Level | Topic | Main design concept |
|------:|---|---|
| 1 | Basic Queue | FIFO, Producer, Consumer |
| 2 | Concurrent Queue | Mutex, `condition_variable`, blocking `pull()` |
| 3 | ACK + In-flight | ACK, timeout, redelivery |
| 4 | Retry + Backoff + DLQ | Retry metadata, priority queue, exponential backoff, DLQ |
| 5 | Multiple Queues | Queue management, create/delete, queue registry |
| 6 | Topics / Pub-Sub | Topic → subscriptions → independent consumers |
| 7 | Consumer Groups | Competing consumers within a group |
| 8 | Ordering | Per-queue / per-key ordering, partitioning |
| 9 | Message Visibility & Delivery Semantics | At-most-once vs at-least-once, duplicate delivery |
| 10 | Idempotency | Deduplication / idempotent consumers |
| 11 | Priority Messages | Priority queue for **delivery**, not just retries |
| 12 | Delayed Messages | Deliver at a future time |
| 13 | Rate Limiting / Backpressure | Stop producers/consumers from overwhelming the system |
| 14 | Scaling | Partitions/sharding, multiple queue workers |
| 15 | Persistence | Disk/storage, not only in-memory |
| 16 | Failure Recovery | Consumer/worker crashes, queue restart, recovery |
| 17 | Observability | Metrics, queue depth, lag, retry count, failures |
| 18 | Advanced Reliability | Circuit breakers, poison messages, DLQ replay |
| 19 | Distributed Queue | Multiple queue nodes, coordination, ownership |
| 20 | Production-Level Design | Combine everything and defend trade-offs |

Work top-down in an interview: stop when the requirement is covered; don’t jump to 19 before ACK exists.
