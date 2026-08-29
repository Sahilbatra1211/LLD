Put this in README.md:

# Strategy Design Pattern

## Intent

> Define a family of algorithms/behaviors, encapsulate each one, and make them interchangeable at runtime.

In simple terms:

> **Separate behaviors that vary and make them interchangeable.**

---

# Duck Example

Suppose we have different types of ducks.

Initially, we might do this:

```cpp
class Duck {
public:
    void swim() {
        // All ducks can swim
    }

    void quack() {
        // All ducks can quack
    }

    void display() {
        // Display duck
    }
};

Now we introduce flying:

class Duck {
public:
    void fly() {
        // Fly
    }
};

The problem is that not every duck can fly.

For example:

Mallard Duck → Can fly
Rubber Duck  → Cannot fly

If we put fly() inside the base Duck class, we may end up with:

class RubberDuck : public Duck {
public:
    void fly() {
        // Do nothing / throw exception
    }
};

This is bad design.

The Problem

Duck behaviors can vary independently.

For example:

Flying behavior
    ├── FlyWithWings
    └── CannotFly

Quacking behavior
    ├── Quack
    ├── Squeak
    └── MuteQuack

We don't want to put all these behaviors directly inside Duck.

Instead, we should separate the behaviors that vary.

Strategy Pattern Solution

Create interfaces for the behaviors:

class FlyBehavior {
public:
    virtual void fly() = 0;
    virtual ~FlyBehavior() = default;
};

class QuackBehavior {
public:
    virtual void quack() = 0;
    virtual ~QuackBehavior() = default;
};

Then create different strategies:

FlyBehavior
    ├── FlyWithWings
    └── NoFly

QuackBehavior
    ├── NormalQuack
    ├── Squeak
    └── MuteQuack

The Duck class has these behaviors instead of inheriting them.

class Duck {
protected:
    FlyBehavior* flyBehavior;
    QuackBehavior* quackBehavior;

public:
    void performFly();
    void performQuack();
};
Composition Instead of Inheritance

This is the most important idea in this example.

Instead of:

Duck
 ↑
MallardDuck
 ↑
RubberDuck

and putting behavior into the inheritance hierarchy, we use composition:

              Duck
             /    \
            ↓      ↓
    FlyBehavior   QuackBehavior
        /  \        /   |   \
       /    \      /    |    \
    Fly    NoFly Quack Squeak Mute

The Duck has a flying behavior.

The Duck has a quacking behavior.

This is composition.

Runtime Behavior Change

The Strategy Pattern allows us to change behavior at runtime.

For example:

duck.setFlyBehavior(new FlyWithWings());

Later:

duck.setFlyBehavior(new NoFly());

The Duck itself doesn't change.

Only its strategy changes.

Why Use Strategy Pattern?

Without Strategy:

Duck
 ├── fly()
 ├── quack()
 ├── if conditions...
 ├── more behavior...
 └── more behavior...

As behaviors increase, the class becomes complicated.

With Strategy:

Duck
 ├── FlyBehavior
 └── QuackBehavior

Each behavior is encapsulated separately.

Strategy Pattern Structure

The general structure is:

             Context
                |
                | has
                ↓
             Strategy
             /      \
            /        \
   Strategy A      Strategy B

In our example:

              Duck
             /    \
            /      \
           ↓        ↓
    FlyBehavior  QuackBehavior
       /   \       /   |   \
      ↓     ↓     ↓    ↓    ↓
    Fly    NoFly Quack Squeak Mute

Duck is the Context.

FlyBehavior and QuackBehavior are Strategies.

FlyWithWings, NoFly, Quack, Squeak, and MuteQuack are Concrete Strategies.

Advantages
1. Removes large if/else or switch statements

Instead of:

if (type == "MALLARD") {
    // fly
}
else if (type == "RUBBER") {
    // don't fly
}

we use polymorphism.

2. Behaviors can be changed independently

Flying behavior can change without modifying quacking behavior.

3. Easy to add new behavior

For example:

class FlyWithJetpack : public FlyBehavior {
public:
    void fly() override {
        // Fly using jetpack
    }
};

No change is required in Duck.

4. Follows Open/Closed Principle

We can add new strategies without modifying the existing Duck class.

5. Favors Composition Over Inheritance

Instead of creating many subclasses for every combination of behaviors, we compose objects from different strategies.

When to Use Strategy Pattern

Use Strategy when:

You have multiple ways of performing an operation.
These algorithms/behaviors can vary independently.
You want to switch behavior at runtime.
You have large if/else or switch statements based on behavior/type.
You want to avoid creating many subclasses for different combinations of behavior.
Strategy vs Inheritance
Inheritance

Behavior is determined by the class hierarchy:

Duck
 ├── MallardDuck
 ├── RubberDuck
 └── WoodenDuck

This can become difficult when behaviors vary independently.

For example:

Duck Type × Flying Type × Quacking Type

can result in many combinations.

Strategy

Behavior is composed:

Duck
 ├── FlyBehavior
 └── QuackBehavior

This allows combinations such as:

MallardDuck
    + FlyWithWings
    + NormalQuack

RubberDuck
    + NoFly
    + Squeak
Strategy Pattern and SOLID
Open/Closed Principle

New behaviors can be added by creating new strategy classes.

New Behavior
     ↓
New Strategy
     ↓
Existing Duck remains unchanged
Single Responsibility Principle

Each strategy has one responsibility.

FlyWithWings
    ↓
Flying behavior

Squeak
    ↓
Quacking behavior
Dependency Inversion Principle

Duck depends on abstractions:

FlyBehavior
QuackBehavior

rather than concrete implementations.

Interview Answer

If asked:

What is the Strategy Design Pattern?

Say:

"Strategy Pattern defines a family of interchangeable algorithms or behaviors, encapsulates each one behind an abstraction, and allows the context to use different strategies without changing its own code. In the Duck example, flying and quacking are separated into behavior interfaces, and different ducks can be composed with different implementations."

One-Line Memory Trick

Strategy = Encapsulate what varies and make it interchangeable.

Or remember:

What varies?
    ↓
Separate it
    ↓
Create interface
    ↓
Create multiple implementations
    ↓
Compose the behavior into the main class
Classic Duck Example
Duck
  |
  +---- FlyBehavior
  |        |
  |        +---- FlyWithWings
  |        +---- NoFly
  |
  +---- QuackBehavior
           |
           +---- Quack
           +---- Squeak
           +---- MuteQuack

The key idea:

The Duck doesn't implement flying or quacking. It delegates those behaviors to strategy objects.