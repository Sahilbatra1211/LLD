# Liskov Substitution Principle (LSP)

## Definition

> Objects of a child/derived class should be able to replace objects of the parent/base class without breaking the correctness of the program.

### Simple way to remember

> **"Child should be substitutable for parent."**

---

## Simple Example

```cpp
class Bird {
public:
    virtual void fly() = 0;
};

class Sparrow : public Bird {
public:
    void fly() override {
        // Sparrow can fly
    }
};
```

This makes sense because `Sparrow` can behave like a `Bird`:

```cpp
Bird* bird = new Sparrow();
bird->fly();   // Works correctly
```

But:

```cpp
class Penguin : public Bird {
public:
    void fly() override {
        throw runtime_error("Penguin cannot fly");
    }
};
```

Now:

```cpp
Bird* bird = new Penguin();
bird->fly();   // Breaks the expected behavior
```

The problem is that `Bird` promised:

> Any `Bird` can `fly()`.

But `Penguin` cannot satisfy that contract.

---

## Better Design

```cpp
class Bird {
};

class FlyingBird : public Bird {
public:
    virtual void fly() = 0;
};

class Sparrow : public FlyingBird {
public:
    void fly() override {
        // fly
    }
};

class Penguin : public Bird {
};
```
