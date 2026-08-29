# Linker and `#pragma once`

## 1. What exactly does the linker do?

Let's use the simplest possible example.

```cpp
// Duck.h
#pragma once

class Duck {
public:
    void fly();
};
```

Notice:

```cpp
void fly();
```

This is only a **declaration**.

It tells the compiler:

> There exists a function called `Duck::fly()`. You don't need to know its implementation right now.

```cpp
// Duck.cpp
#include "Duck.h"

void Duck::fly() {
    // actual implementation
}
```

This contains the **definition**:

```cpp
void Duck::fly() {
    // ...
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

Now let's compile.

---

### Step 1 — Preprocessor

For `main.cpp`, `#include "Duck.h"` gets essentially replaced with the contents of `Duck.h`.

So the compiler effectively receives:

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

Notice something important: the implementation of `fly()` is **not** here.

We only have:

```cpp
void fly();
```

---

### Step 2 — Compiler

The compiler compiles this translation unit:

```text
main.cpp
   ↓
preprocessor
   ↓
translation unit
   ↓
compiler
   ↓
main.o
```

The compiler sees `duck.fly();` and says:

> Okay, `Duck::fly()` has been declared. I know this function exists.

But it doesn't have the implementation.

So conceptually, `main.o` contains something like:

```text
main()
  ↓
needs Duck::fly()
```

It has a **reference** to `Duck::fly()`.

Now separately:

```text
Duck.cpp
   ↓
preprocessor
   ↓
translation unit
   ↓
compiler
   ↓
Duck.o
```

`Duck.o` contains the actual compiled implementation of `Duck::fly()`.

Conceptually:

```text
Duck.o

Duck::fly()
{
    actual machine code...
}
```

---

### Step 3 — Linker

Now we have:

```text
main.o
Duck.o
```

The linker basically says:

> Let me resolve all the things that one object file needs from other object files.

`main.o` says: **I need `Duck::fly()`**

`Duck.o` says: **I provide `Duck::fly()`**

So the linker connects them:

```text
             main.o
               |
               | needs
               ↓
          Duck::fly()
               ↑
               | provided by
               |
             Duck.o
```

Then the linker produces the final executable:

```text
main.o
   \
    \
     → LINKER → app.exe
    /
   /
Duck.o
```

So yes, your understanding is correct:

> The compiler creates object files independently. The linker then resolves references between those object files and other libraries, combines everything, and produces the executable.

### One small correction

The linker isn't really *"linking the header."*

Headers are **not** separate compiled entities that the linker links.

The header's declarations were used during compilation, while the actual definitions usually come from `.cpp` files that were separately compiled into `.o` files.

Think:

```text
.h
 ↓
Declarations
 ↓
Compiler needs these to understand code

.cpp
 ↓
Definitions
 ↓
Compiler creates object code

.o + .o + libraries
 ↓
Linker
 ↓
.exe
```

---

## 2. Now let's understand `#pragma once`

This one is much easier once you understand what `#include` actually does.

Remember: `#include "Duck.h"` doesn't mean *"Link `Duck.h`."*

It essentially means:

> Copy the contents of `Duck.h` here before compilation.

Now imagine this.

```cpp
// Duck.h
class Duck {
public:
    void fly();
};
```

```cpp
// A.h
#include "Duck.h"
```

```cpp
// B.h
#include "Duck.h"
```

```cpp
// main.cpp
#include "A.h"
#include "B.h"
```

Now follow what happens.

`main.cpp` includes `A.h`:

```text
main.cpp
   ↓
A.h
   ↓
Duck.h
```

So we get:

```cpp
class Duck {
public:
    void fly();
};
```

Then `main.cpp` includes `B.h`:

```text
main.cpp
   ↓
B.h
   ↓
Duck.h
```

So we get the same class declaration again.

Conceptually, the compiler sees:

```cpp
class Duck {
public:
    void fly();
};

class Duck {
public:
    void fly();
};

int main() {
    ...
}
```

And now we have a problem: **`Duck` has been declared twice in the same translation unit.**

---

### `#pragma once` fixes this

Put `#pragma once` at the top of `Duck.h`:

```cpp
#pragma once

class Duck {
public:
    void fly();
};
```

Now the preprocessor essentially says:

```text
First time I see Duck.h?
    ↓
Include it.

Second time I see Duck.h?
    ↓
Skip it.
```

So:

```text
A.h
 ↓
Duck.h → included

B.h
 ↓
Duck.h → already included → SKIP
```

The compiler ultimately sees:

```cpp
class Duck {
public:
    void fly();
};

int main() {
    ...
}
```

only once.

---

### Why does this happen so often?

Because real projects have include chains.

For example:

```text
main.cpp
 ├── Duck.h
 │    └── Animal.h
 │
 └── Game.h
      └── Animal.h
```

So `Animal.h` gets included twice:

```text
main.cpp
   |
   +---- Duck.h
   |       |
   |       +---- Animal.h
   |
   +---- Game.h
           |
           +---- Animal.h
```

Without include protection:

```text
Animal.h
Animal.h
```

gets inserted twice.

With `#pragma once`, the second inclusion is ignored.

---

### One important distinction

`#pragma once` does **not** mean:

> Compile this header once for the entire project.

It means:

> Within this translation unit, include this header's contents only once.

Suppose:

```text
main.cpp → Duck.h
Game.cpp → Duck.h
Player.cpp → Duck.h
```

`Duck.h` is still processed once for `main.cpp`, once for `Game.cpp`, and once for `Player.cpp`, because these are three different translation units.

So:

```text
              Duck.h
             /   |   \
            ↓    ↓    ↓
        main.cpp Game.cpp Player.cpp
            ↓      ↓       ↓
         main.o  Game.o  Player.o
```

`#pragma once` doesn't prevent that.

---

### The whole picture

Now connect both concepts:

```text
                     Duck.h
                       |
                #pragma once
                       |
          +------------+------------+
          |                         |
       main.cpp                  Duck.cpp
          |                         |
       #include                  #include
          |                         |
          ↓                         ↓
   Preprocessor              Preprocessor
          |                         |
          ↓                         ↓
 Translation Unit            Translation Unit
          |                         |
       Compiler                  Compiler
          ↓                         ↓
       main.o                    Duck.o
          |                         |
          +-----------+-------------+
                      |
                    Linker
                      |
                      ↓
                   app.exe
```

And the roles are:

```text
HEADER
  ↓
Declarations / interfaces
  ↓
Helps compiler understand code

CPP
  ↓
Definitions / implementation
  ↓
Gets compiled into object code

OBJECT FILES
  ↓
Compiled pieces of the program

LINKER
  ↓
Connects references to definitions
  ↓
Combines object files + libraries
  ↓
Executable
```

### And `#pragma once`?

It's simply a header protection mechanism:

```text
Same header encountered multiple times
                 ↓
           #pragma once
                 ↓
       Include it only once
       per translation unit
```
