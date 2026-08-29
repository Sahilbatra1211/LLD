# C++ Compilation Lifecycle

## 1. Start with a simple project

Suppose you have:

```text
MyProject/
├── main.cpp
├── Duck.h
├── Duck.cpp
└── FlyBehavior.h
```

```cpp
// Duck.h
#pragma once

class Duck {
public:
    void fly();
};
```

```cpp
// Duck.cpp
#include "Duck.h"

void Duck::fly() {
    // implementation
}
```

```cpp
// main.cpp
#include "Duck.h"

int main() {
    Duck duck;
    duck.fly();
}
```

Now what happens when you run:

```bash
g++ main.cpp Duck.cpp -o app
```

There are several stages.

---

## 2. The C++ compilation lifecycle

```text
.cpp files
    ↓
Preprocessor
    ↓
Compiler
    ↓
Object files
    ↓
Linker
    ↓
Executable
```

More specifically:

```text
             main.cpp
                ↓
          Preprocessor
                ↓
            main.i
                ↓
             Compiler
                ↓
            main.o
                \
                 \
                  → Linker → app.exe
                 /
                /
            Duck.o
                ↑
             Compiler
                ↑
          Preprocessor
                ↑
             Duck.cpp
```

The important part is: **`main.cpp` and `Duck.cpp` are compiled independently.**

---

## 3. What does the preprocessor do?

This is where `.h` files become important.

```cpp
// Duck.h
class Duck {
public:
    void fly();
};
```

```cpp
// main.cpp
#include "Duck.h"

int main() {
    Duck duck;
    duck.fly();
}
```

The preprocessor essentially performs `#include "Duck.h"` by inserting the contents of `Duck.h`.

Conceptually, this:

```cpp
#include "Duck.h"

int main() {
    Duck duck;
    duck.fly();
}
```

becomes:

```cpp
class Duck {
public:
    void fly();
};

int main() {
    Duck duck;
    duck.fly();
}
```

The compiler doesn't really think: *"Oh, this came from a header."*

It sees the expanded source code.

---

## 4. Each `.cpp` becomes a Translation Unit

This is an important term.

After preprocessing:

```text
main.cpp
   ↓
preprocessed source
   ↓
Translation Unit
```

```text
Duck.cpp
   ↓
preprocessed source
   ↓
Translation Unit
```

Each translation unit is compiled separately.

```text
main.cpp
   ↓
Translation Unit #1
   ↓
main.o
```

```text
Duck.cpp
   ↓
Translation Unit #2
   ↓
Duck.o
```

---

## 5. Compiler creates object files

The compiler takes `main.cpp` and generates something like `main.o`.

And `Duck.cpp` becomes `Duck.o`.

These `.o` files contain machine code, symbols, metadata, etc.

On Windows with MinGW/MSYS2 you may see `main.o` and `Duck.o`, or `.obj` depending on the toolchain.

---

## 6. Then comes the linker

Now we have:

```text
main.o
Duck.o
```

But `main.o` contains a call to `duck.fly();`

The compiler knows *"There is a `Duck::fly()` function"* because it saw:

```cpp
void fly();
```

But the actual implementation is in `Duck.cpp`.

So the object file might effectively say: **I need `Duck::fly()`**

Then the linker looks through `Duck.o` and finds `Duck::fly()`. It connects them.

Conceptually:

```text
main.o
   |
   | needs Duck::fly()
   ↓
Linker
   ↑
   | finds Duck::fly()
   |
Duck.o
```

Then `app.exe` is produced.

---

## 7. Now the important part: recompilation

Suppose you have:

```text
main.cpp
Duck.cpp
Duck.h
```

Initially:

```text
main.cpp → main.o
Duck.cpp → Duck.o
              ↓
            linker
              ↓
           app.exe
```

Now you change only:

```cpp
// Duck.cpp
void Duck::fly() {
    cout << "Flying with wings";
}
```

You didn't change `Duck.h` or `main.cpp`.

So ideally, we only need:

```text
Duck.cpp
   ↓
Duck.o
   ↓
link
   ↓
app.exe
```

We do **not** need to compile `main.cpp` again.

**Why?** Because `main.cpp` depends on the interface `void fly();` and that didn't change.

This is the advantage of `.cpp` implementations.

---

## 8. Now put the implementation in the `.h`

Suppose instead you write:

```cpp
// Duck.h
#pragma once

class Duck {
public:
    void fly() {
        // implementation
    }
};
```

```cpp
// main.cpp
#include "Duck.h"

int main() {
    Duck duck;
    duck.fly();
}
```

Remember what the preprocessor does? It effectively creates:

```cpp
class Duck {
public:
    void fly() {
        // implementation
    }
};

int main() {
    Duck duck;
    duck.fly();
}
```

So `main.cpp` now contains the entire implementation of `Duck`.

---

## 9. Now imagine 100 `.cpp` files

Suppose your project looks like:

```text
Project/
├── main.cpp
├── Game.cpp
├── Player.cpp
├── Enemy.cpp
├── Renderer.cpp
├── Physics.cpp
├── Network.cpp
├── Database.cpp
├── ...
└── Duck.h
```

And all 100 `.cpp` files include:

```cpp
#include "Duck.h"
```

If `Duck.h` contains:

```cpp
class Duck {
public:
    void fly() {
        // 500 lines
    }
};
```

then every translation unit gets those 500 lines.

Conceptually:

```text
main.cpp
    +
Duck.h implementation
    ↓
main.o

Game.cpp
    +
Duck.h implementation
    ↓
Game.o

Player.cpp
    +
Duck.h implementation
    ↓
Player.o

Enemy.cpp
    +
Duck.h implementation
    ↓
Enemy.o
```

The implementation has to be processed repeatedly.

---

## 10. Now change one line in `Duck.h`

Suppose you change:

```cpp
cout << "Flying";
```

to:

```cpp
cout << "Flying!";
```

You changed `Duck.h`.

Who depends on `Duck.h`? Potentially:

```text
main.cpp
Game.cpp
Player.cpp
Enemy.cpp
Renderer.cpp
Physics.cpp
Network.cpp
Database.cpp
...
```

Therefore all those translation units may need recompilation.

```text
             Duck.h
               |
       +-------+-------+
       |       |       |
       ↓       ↓       ↓
    main.cpp Game.cpp Player.cpp
       ↓       ↓       ↓
    main.o   Game.o   Player.o
```

That's what we mean by:

> Changing a header can trigger widespread recompilation.

---

## 11. Compare `.h` implementation vs `.cpp` implementation

### Implementation in `.h`

```text
Duck.h
  ↓
included by 100 .cpp files
  ↓
100 translation units affected
  ↓
100 files potentially recompiled
```

### Implementation in `.cpp`

```text
Duck.h
  ↓
declaration only
  ↓
100 .cpp files use it

Duck.cpp
  ↓
implementation
  ↓
only Duck.cpp needs recompilation
```

That's the core concept.

---

## 12. This is why large C++ projects care about this

Imagine Microsoft Windows or a huge game engine with 10,000 `.cpp` files.

Suppose `Common.h` is included by 8,000 `.cpp` files.

And you change one tiny implementation detail inside `Common.h`.

Potentially:

```text
Common.h changed
       ↓
8,000 translation units invalidated
       ↓
8,000 compilations
       ↓
very slow build
```

This can turn a **10 second build** into **several minutes**, or worse in very large systems.

---

## 13. But there's an even bigger problem

It's not just implementation size. Headers often include other headers.

For example:

```cpp
// Duck.h
#include <iostream>
#include <vector>
#include "Animal.h"
#include "Movement.h"
#include "Physics.h"
#include "Renderer.h"
```

Then:

```text
main.cpp
    ↓
Duck.h
    ↓
Animal.h
    ↓
Physics.h
    ↓
Renderer.h
    ↓
...
```

Your simple `#include "Duck.h"` can indirectly pull in a huge amount of code.

That's called a **header dependency problem**.

---

## 14. Why `.cpp` helps

You can instead have:

```cpp
// Duck.h
#pragma once

class Duck {
public:
    void fly();
};
```

Very lightweight.

Then:

```cpp
// Duck.cpp
#include "Duck.h"
#include <iostream>
#include "HugePhysicsLibrary.h"

void Duck::fly() {
    // implementation
}
```

Now users of `#include "Duck.h"` don't need to process `HugePhysicsLibrary.h` just to know `void fly();`

This reduces dependencies.

---

## 15. What `#pragma once` does

You might wonder:

> If headers are copied into every `.cpp`, won't including the same header multiple times cause duplicate definitions?

That's one reason we use `#pragma once`.

```cpp
// Duck.h
#pragma once

class Duck {
};
```

It tells the compiler/preprocessor:

> Include this header only once **per translation unit**.

It does **not** mean: *Compile this header only once for the entire project.*

That's an important distinction.

---

## 16. CMake doesn't change this fundamental process

When you do:

```bash
cmake --build build
```

CMake figures out:

- Which files changed?
- Which files depend on them?
- What needs rebuilding?

For example:

```text
Duck.cpp changed
       ↓
Recompile Duck.cpp
       ↓
Duck.o
       ↓
Link
```

But:

```text
Duck.h changed
       ↓
Check who includes Duck.h
       ↓
Recompile affected .cpp files
       ↓
Link
```

CMake/build systems automate this dependency tracking.

---

## 17. A real example

Suppose:

```text
src/
├── main.cpp
├── Duck.cpp
├── MallardDuck.cpp
├── RubberDuck.cpp
└── Duck.h
```

### Version A — implementation in `.cpp`

```cpp
// Duck.h
class Duck {
public:
    void fly();
};
```

```cpp
// Duck.cpp
#include "Duck.h"

void Duck::fly() {
    // implementation
}
```

If you change:

```cpp
// Duck.cpp
void Duck::fly() {
    // NEW implementation
}
```

Build system can do:

```text
Duck.cpp
   ↓
Duck.o

main.o             ─┐
MallardDuck.o       ├──→ Linker → executable
RubberDuck.o       ─┘
Duck.o             ──
```

Only `Duck.cpp` needs recompilation.

---

## 18. Version B — implementation in `.h`

```cpp
// Duck.h
class Duck {
public:
    void fly() {
        // implementation
    }
};
```

Now:

```text
main.cpp
     ↓
Duck.h
     ↓
main.o

MallardDuck.cpp
     ↓
Duck.h
     ↓
MallardDuck.o

RubberDuck.cpp
     ↓
Duck.h
     ↓
RubberDuck.o
```

Change `Duck.h`:

```text
Duck.h changed
     ↓
main.cpp affected
MallardDuck.cpp affected
RubberDuck.cpp affected
     ↓
All potentially recompiled
```

That's the disadvantage.

---

## 19. One important correction

You might think:

> If implementation is in `.h`, won't it cause multiple-definition linker errors?

Sometimes, yes, depending on what you're putting in the header.

For example, this is dangerous:

```cpp
// foo.h
void hello() {
    // ...
}
```

Included by multiple `.cpp` files, this can create multiple definitions under normal non-inline rules.

But member functions defined inside the class definition are **implicitly inline**:

```cpp
class Duck {
public:
    void fly() {
        // ...
    }
};
```

So this is generally fine.

Templates also commonly need their implementations in headers because the compiler needs to see the implementation when instantiating them.

---

## 20. So why do we use `.h` + `.cpp`?

The general philosophy is:

```text
.h
 ↓
"What does this class expose?"
 ↓
Interface / declaration

.cpp
 ↓
"How does this class implement it?"
 ↓
Implementation
```

Example:

```cpp
// Duck.h
class Duck {
public:
    void fly();
    void quack();
};
```

```cpp
// Duck.cpp
#include "Duck.h"

void Duck::fly() {
    // how flying works
}

void Duck::quack() {
    // how quacking works
}
```

This provides:

```text
Smaller headers
      ↓
Fewer dependencies
      ↓
Less recompilation
      ↓
Faster builds
      ↓
Better separation
```

---

## 21. And now connect this to your Strategy project

You currently have:

```text
Strategy/
└── src/
    ├── Duck.h
    ├── FlyBehavior.h
    ├── FlyWithWings.h
    ├── NoFly.h
    ├── QuackBehavior.h
    ├── Quack.h
    ├── Squeak.h
    ├── MuteQuack.h
    └── main.cpp
```

For these tiny classes, putting implementations in `.h` is completely reasonable.

You're learning the pattern, and build time is irrelevant.

But when we build something like `ParkingLot/` with 20+ classes, I'd prefer:

```text
include/
    ParkingLot.h
    ParkingSpot.h
    Vehicle.h
    Ticket.h

src/
    ParkingLot.cpp
    ParkingSpot.cpp
    Vehicle.cpp
    Ticket.cpp
    main.cpp
```

Then you'll actually experience why the separation exists.

---

## 22. The mental model you should remember

This is probably the most important part:

```text
             C++ PROJECT
                  |
          +-------+-------+
          |               |
       main.cpp        Duck.cpp
          |               |
      preprocess       preprocess
          |               |
          ↓               ↓
    Translation Unit Translation Unit
          |               |
       compile          compile
          ↓               ↓
       main.o           Duck.o
          \               /
           \             /
             ↓         ↓
                LINKER
                  ↓
              app.exe
```

And:

```text
Change Duck.cpp
      ↓
Usually only Duck.cpp recompiles
```

whereas:

```text
Change Duck.h
      ↓
Every .cpp that depends on Duck.h
may need recompilation
```

### The one sentence to remember

> `.h` files are included into translation units, so changing a widely included header can force many `.cpp` files to be recompiled; putting implementation in `.cpp` localizes implementation changes and reduces build-time dependencies.
