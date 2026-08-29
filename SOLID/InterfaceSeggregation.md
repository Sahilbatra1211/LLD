Interface Segregation Principle (ISP) says:

A class should not be forced to depend on methods that it does not use.

Simple way to remember

"Don't create fat interfaces. Keep interfaces small and focused."

❌ Bad Example
class Worker {
public:
    virtual void work() = 0;
    virtual void eat() = 0;
};

Now suppose we have a Robot:

class Robot : public Worker {
public:
    void work() override {
        // Robot works
    }

    void eat() override {
        // Robot doesn't eat!
    }
};

The Robot is forced to implement eat() even though it doesn't need it.

That's an ISP violation.

✅ Good Design

Split the interface:

class Workable {
public:
    virtual void work() = 0;
};

class Eatable {
public:
    virtual void eat() = 0;
};

Now:

class Human : public Workable, public Eatable {
public:
    void work() override {}
    void eat() override {}
};

class Robot : public Workable {
public:
    void work() override {}
};