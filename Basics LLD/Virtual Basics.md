# Virtual Basics

## 1. First: What problem does `virtual` solve?

Start with inheritance.

```cpp
#include <iostream>
using namespace std;

class Animal {
public:
    void speak() {
        cout << "Animal speaks\n";
    }
};

class Dog : public Animal {
public:
    void speak() {
        cout << "Dog barks\n";
    }
};
```

Now:

```cpp
Dog dog;
dog.speak();
```

Output:

```text
Dog barks
```

Nothing surprising.

But now:

```cpp
Animal* animal = new Dog();
animal->speak();
```

What do you expect?

You might think `Dog barks` because the actual object is a `Dog`.

But the output is:

```text
Animal speaks
```

**Why?** That's the problem `virtual` solves.

---

## 2. Static type vs actual type

Look at:

```cpp
Animal* animal = new Dog();
```

There are two different types involved.

### Static type

The type of the pointer: `Animal*`

### Dynamic type

The actual object: `Dog`

So:

```text
Pointer type → Animal*
Actual object → Dog
```

Without `virtual`, C++ chooses the function based on the **static type**.

```text
Animal*
   ↓
speak()
   ↓
Animal::speak()
```

This is called **static binding / early binding**.

---

## 3. Add `virtual`

Now change:

```cpp
class Animal {
public:
    virtual void speak() {
        cout << "Animal speaks\n";
    }
};
```

And:

```cpp
class Dog : public Animal {
public:
    void speak() override {
        cout << "Dog barks\n";
    }
};
```

Now:

```cpp
Animal* animal = new Dog();
animal->speak();
```

Output:

```text
Dog barks
```

**Why?** Because `virtual` tells C++:

> When this function is called through a base-class reference/pointer, determine which implementation to call based on the actual object at runtime.

That's **dynamic binding / runtime polymorphism**.

---

## 4. The fundamental difference

Without `virtual`:

```text
Animal* animal = new Dog();

animal->speak();
      ↓
Look at pointer type
      ↓
Animal
      ↓
Animal::speak()
```

With `virtual`:

```text
Animal* animal = new Dog();

animal->speak();
      ↓
Look at actual object
      ↓
Dog
      ↓
Dog::speak()
```

This is the entire reason `virtual` exists.

---

## 5. Why is this useful?

This is where LLD starts becoming interesting.

Suppose:

```cpp
class Animal {
public:
    virtual void speak() = 0;
};
```

And:

```cpp
class Dog : public Animal {
public:
    void speak() override {
        cout << "Bark\n";
    }
};

class Cat : public Animal {
public:
    void speak() override {
        cout << "Meow\n";
    }
};
```

Now you can write:

```cpp
void makeAnimalSpeak(Animal& animal) {
    animal.speak();
}
```

And:

```cpp
Dog dog;
Cat cat;

makeAnimalSpeak(dog);
makeAnimalSpeak(cat);
```

Output:

```text
Bark
Meow
```

The function doesn't need to know whether it received:

```text
Dog
Cat
Cow
Lion
...
```

It only knows `Animal`.

This is **polymorphism**.

---

## 6. This is exactly what we did in Strategy

Remember:

```cpp
class FlyBehavior {
public:
    virtual void fly() const = 0;
};
```

Then:

```cpp
class FlyWithWings : public FlyBehavior {
public:
    void fly() const override {
        cout << "Flying with wings\n";
    }
};
```

and:

```cpp
class NoFly : public FlyBehavior {
public:
    void fly() const override {
        cout << "I cannot fly\n";
    }
};
```

Then:

```cpp
unique_ptr<FlyBehavior> flyBehavior;
```

This pointer could actually point to `FlyWithWings` or `NoFly`.

When we do:

```cpp
flyBehavior->fly();
```

C++ needs to figure out:

> Should I call `FlyWithWings::fly()` or `NoFly::fly()`?

Because `fly()` is `virtual`, it chooses based on the actual object.

That's the mechanism that makes Strategy work.

---

## 7. What does `= 0` mean?

This:

```cpp
virtual void fly() = 0;
```

is called a **pure virtual function**.

It means:

> Derived classes must provide an implementation.

So:

```cpp
class FlyBehavior {
public:
    virtual void fly() = 0;
};
```

is an **abstract class**.

You cannot do:

```cpp
FlyBehavior f; // ❌
```

But you can:

```cpp
FlyWithWings f; // ✅
```

because `FlyWithWings` implements `fly()`.

---

## 8. `virtual` vs `override`

You'll commonly see:

```cpp
class Animal {
public:
    virtual void speak() {
    }
};

class Dog : public Animal {
public:
    void speak() override {
    }
};
```

These keywords have different purposes.

### `virtual`

Used in the base class:

```cpp
virtual void speak();
```

It says:

> This function participates in runtime polymorphism.

### `override`

Used in the derived class:

```cpp
void speak() override;
```

It says:

> I intend to override a virtual function from the base class.

And this is actually useful because the compiler catches mistakes.

For example:

```cpp
class Animal {
public:
    virtual void speak() const {
    }
};

class Dog : public Animal {
public:
    void speak() override {  // ❌ compiler error
    }
};
```

**Why?**

Base has `speak() const`. Derived has `speak()`. Those aren't the same signature.

Without `override`, you could accidentally create a new function instead of overriding the base function.

So always use `override` when overriding.

---

## 9. Now the important destructor issue

Consider:

```cpp
class Animal {
public:
    virtual void speak() {}
};

class Dog : public Animal {
public:
    ~Dog() {
        cout << "Dog destroyed\n";
    }
};
```

And:

```cpp
Animal* animal = new Dog();
delete animal;
```

This is dangerous if the base destructor isn't `virtual`.

You generally want:

```cpp
class Animal {
public:
    virtual void speak() {}

    virtual ~Animal() = default;
};
```

Now:

```cpp
Animal* animal = new Dog();
delete animal;
```

correctly destroys:

```text
Dog destructor
      ↓
Animal destructor
```

This is especially important for interfaces/base classes intended for polymorphic use.

That's why our Strategy interface had:

```cpp
class FlyBehavior {
public:
    virtual void fly() const = 0;
    virtual ~FlyBehavior() = default;
};
```

---

## 10. Why does `virtual` have a runtime cost?

You may hear about a **vtable**.

For a class with virtual functions, most implementations use something conceptually like:

```text
Object
┌─────────────────┐
│ vptr ───────────────→ vtable
│ other data       │
└─────────────────┘

vtable
┌─────────────────┐
│ speak → Dog::speak
│ destructor → ...
└─────────────────┘
```

So when you do:

```cpp
animal->speak();
```

the program can effectively do:

```text
animal
  ↓
vptr
  ↓
vtable
  ↓
Dog::speak()
```

This is why the call can be resolved at runtime.

You don't need to memorize the exact compiler implementation—the C++ standard doesn't require specifically "vtable + vptr"—but vtable is the common implementation and absolutely worth knowing for interviews.

---

## 11. Why not make everything `virtual`?

Good question.

You don't need `virtual` everywhere.

Use it when you actually need runtime polymorphism.

For example:

```cpp
class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }
};
```

There's no reason for `virtual int add(...)` unless derived classes need to provide different implementations and you want calls through the base abstraction to dispatch dynamically.

---

## 12. One subtle but VERY important point

`virtual` matters when you're using a **base pointer/reference**.

Example:

```cpp
Dog dog;
dog.speak();
```

Even if `speak()` is `virtual`, there's no interesting runtime dispatch here.

The compiler already knows: `dog → Dog`

The important case is:

```cpp
Animal* animal = &dog;
animal->speak();
```

or:

```cpp
Animal& animal = dog;
animal.speak();
```

Now the static type is `Animal` but the actual object is `Dog`, and `virtual` allows the call to resolve to `Dog::speak()`.

---

## 13. Pointer vs reference

Both work:

```cpp
Animal* a = new Dog();
a->speak();
```

and:

```cpp
Dog dog;
Animal& a = dog;
a.speak();
```

Both can use virtual dispatch.

So remember:

```text
Base pointer + virtual
        ↓
runtime polymorphism

Base reference + virtual
        ↓
runtime polymorphism
```

---

## 14. The most important interview example

If the interviewer gives:

```cpp
class Base {
public:
    void show() {
        cout << "Base";
    }
};

class Derived : public Base {
public:
    void show() {
        cout << "Derived";
    }
};

Base* ptr = new Derived();
ptr->show();
```

Answer: **Base** — because `show()` is not `virtual`.

If:

```cpp
class Base {
public:
    virtual void show() {
        cout << "Base";
    }
};
```

then: **Derived** — because runtime polymorphism is enabled.

---

## 15. Your mental model

Don't memorize a complicated definition.

Remember this:

```text
Inheritance
    ↓
Base pointer/reference can point to derived object
    ↓
Need different behavior depending on actual object?
    ↓
virtual
    ↓
Runtime polymorphism
```

Example:

```cpp
Animal* animal = new Dog();
```

means:

> I am holding a `Dog`, but I want to interact with it as an `Animal`.

Then:

```cpp
animal->speak();
```

with `virtual` means:

> Call the version appropriate for the actual object.

---

## 16. Connection to LLD

This is why you'll constantly see:

```cpp
class Payment {
public:
    virtual void pay() = 0;
    virtual ~Payment() = default;
};
```

then:

```cpp
class CreditCardPayment : public Payment {};
class UPIPayment : public Payment {};
class PayPalPayment : public Payment {};
```

And:

```cpp
unique_ptr<Payment> payment;
```

The code using `Payment` doesn't care about the concrete implementation.

That's the foundation of:

- Strategy
- Factory
- Observer
- State
- Template Method
- Dependency Inversion
- Many LLD designs

---

## The 5 things I want you to remember right now

1. **`virtual`** — enables runtime polymorphism
2. **`Base*` / `Base&`** — can refer to a `Derived` object
3. **virtual function** — call is based on actual object
4. **`= 0`** — pure virtual function; makes class abstract
5. **virtual destructor** — important when deleting derived objects through a base pointer

And the single most important example:

```cpp
class Animal {
public:
    virtual void speak() = 0;
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    void speak() override {
        cout << "Bark\n";
    }
};

Animal* animal = new Dog();
animal->speak();  // Bark
```
