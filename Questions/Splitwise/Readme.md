# Splitwise LLD

## Design rating (current code)

**7 / 10** for an interview **skeleton**. Layers are the right story. Wiring and a few domain details are still loose — fine if you say *design-focused, not fully implemented*.

### What is already strong

- **`Splitwise` is the facade / orchestrator** — like `ParkingLot` / `Game`. `main` talks to it; it does not own every list itself.
- **Services extracted for real reasons:** adding an expense touches *history* (`TransactionService`) and *who owes whom* (`BalanceService`). That is the same call you made for TicketManager vs PaymentService.
- **Stores** (`TransactionStore`, `BalanceStore`) = persistence boundary. Services coordinate; stores hold maps/lists.
- **`SplitStrategy` is justified.** Equal vs unequal vs percent is varying **algorithm**, not a new `Expense` subclass per type.
- **Models are thin:** `User`, `Group`, `Transaction`, `Balance`.
- **`BalanceStore` splits global vs group balances** — interviewers like that (friends vs trip).

### What they will poke

1. **`Splitwise` never injects the services.** `balanceService` / `transactionService` are uninitialized pointers. Add a constructor (or `main` wires them).
2. **`TransactionService` does not own `TransactionStore` in code** — empty service, `addTransaction()` takes no args. Intended: service HAS-A store, `addTransaction(Transaction*)` or build the entity inside the service.
3. **`addSplit` order is good** (record expense, then balances) but `addTransaction()` cannot save who/amount yet.
4. **`Balance` is `user1`, `user2`, `amount`** — say in the interview: *directed debt* (e.g. `user2` owes `user1`). Name fields `from` / `to` or `owedBy` / `owedTo`.
5. **`TransactionType` is INPROGRESS / SETTLED** — that is **status**, not split type. Split type is the strategy. Settle updates balances (and maybe status).
6. **`User*` as `unordered_map` key** works only if you never copy `User` and always use the same pointers. Safer: `userId`.
7. **`settleDebt` is a stub** — mention it: settle reduces/removes `Balance` rows; optionally a SETTLED transaction.
8. Hygiene: absolute `#include` paths, `virtual ~SplitStrategy()`, no getters.

**Interview line:** *Splitwise orchestrates. TransactionService + store = expense log. BalanceService + store = net debts. SplitStrategy computes shares. I didn’t implement every method on purpose.*

---

## Current / intended architecture

```text
main
 └── Splitwise
        ├── TransactionService  →  TransactionStore  →  Transaction[]
        └── BalanceService      →  BalanceStore      →  Balance[] (global + per group)
                                      ↑
                                 SplitStrategy.calculate()
```

```text
addSplit(whoPaid, people, amount, group, strategy)
    ↓
TransactionService.addTransaction(...)     // history
    ↓
BalanceService.addBalance(...)             // strategy → Balance rows → store
```

---

## Full class diagram

Stores are shown **owned by services** even where the `.h` files are still empty — that is the intended wiring.

```mermaid
classDiagram
    class Splitwise {
        -BalanceService* balanceService
        -TransactionService* transactionService
        +addSplit(whoPaid, splitBetween, amount, group, strategy) bool
        +settleDebt(user1, user2, amount) bool
    }

    class TransactionService {
        -TransactionStore* transactionStore
        +addTransaction(...) bool
    }

    class BalanceService {
        -BalanceStore* balanceStore
        +addBalance(whoPaid, splitBetween, amount, group, strategy) bool
    }

    class TransactionStore {
        +save(Transaction*)
        +getAll() vector~Transaction*~
    }

    class BalanceStore {
        -unordered_map~User*, vector~Balance*~~ globalBalances
        -unordered_map~Group*, vector~Balance*~~ groupBalances
        +storeBalances(vector~Balance*~, Group*) bool
    }

    class User {
        -string userName
        -int id
        +User(string, int)
    }

    class Group {
        -vector~User*~ users
        -string groupName
        -int id
        +Group(vector~User*~, string, int)
    }

    class Transaction {
        -User* whoPaid
        -vector~User*~ splitBetween
        -int date
        -TransactionType transactionType
        -int amount
        -SplitStrategy* splitStrategy
        +Transaction(...)
    }

    class Balance {
        -User* user1
        -User* user2
        -int amount
        +Balance(User*, User*, int)
    }

    class TransactionType {
        <<enumeration>>
        INPROGRESS
        SETTLED
    }

    class SplitStrategy {
        <<interface>>
        +calculate(splitBetween, amount) vector~pair~User*,int~~
    }

    class EqualSplitStrategy {
        +calculate(splitBetween, amount) vector~pair~User*,int~~
    }

    class UnequalSplitStrategy {
        +calculate(splitBetween, amount) vector~pair~User*,int~~
    }

    Splitwise --> TransactionService : HAS-A
    Splitwise --> BalanceService : HAS-A
    TransactionService --> TransactionStore : HAS-A
    BalanceService --> BalanceStore : HAS-A
    TransactionStore --> Transaction : stores
    BalanceStore --> Balance : stores
    BalanceStore --> User : map key (global)
    BalanceStore --> Group : map key (group)
    Transaction --> User : whoPaid / splitBetween
    Transaction --> TransactionType : uses
    Transaction --> SplitStrategy : HAS-A
    Balance --> User : user1 / user2 (directed debt)
    Group --> User : HAS-A members
    BalanceService ..> SplitStrategy : uses calculate()
    EqualSplitStrategy --|> SplitStrategy : IS-A
    UnequalSplitStrategy --|> SplitStrategy : IS-A
```

### How to read it

| Piece | Role |
|---|---|
| `Splitwise` | Facade. `addSplit` / `settleDebt` only. |
| `TransactionService` | Create/save expense records. **HAS-A** `TransactionStore`. |
| `BalanceService` | Turn a split into `Balance` rows. **HAS-A** `BalanceStore`. |
| `TransactionStore` | Persistence for `Transaction` (in-memory list/map for LLD). |
| `BalanceStore` | Persistence for debts: global map + per-`Group` map. |
| `SplitStrategy` | How to divide `amount` among people. |
| `Transaction` | One expense event (history). |
| `Balance` | Net “A ↔ B owes X” (simplify later if asked). |
| `Group` | Optional scope for balances. |

Strategy vs service (same as parking lot):

```text
BalanceService          ←  owns the “update who owes whom” workflow
      ↓
SplitStrategy           ←  varying algorithm (equal / unequal)
      ↓
list of (User, share)
      ↓
BalanceStore
```

---

## What is intentionally unimplemented

- `TransactionService.addTransaction` body
- `TransactionStore` API
- `BalanceStore.storeBalances` body
- `EqualSplitStrategy` / `UnequalSplitStrategy` calculate
- `settleDebt`
- `Splitwise` constructor wiring

Say that up front so they don’t grade missing loops as missing design.

---

## Likely follow-ups (don’t build until asked)

| They say | You do |
|---|---|
| Percent / share split | New `SplitStrategy` implementation |
| Simplify debts (min transactions) | Algorithm on `BalanceStore` snapshot — not a new service on day one |
| Settle up | `BalanceService.settle` + maybe SETTLED on transaction |
| Persistence | Store interface + in-memory vs DB (repository) |
| Concurrent groups | Lock per group id, not one global lock |

---

## Interview script (30 seconds)

> Users and groups are models. An expense is a `Transaction` stored via `TransactionService`. Shares come from `SplitStrategy`. `BalanceService` writes directed `Balance` rows into `BalanceStore` (global or group). `Splitwise` only orchestrates those two services. Stores are empty on purpose; the dependencies are the design.
