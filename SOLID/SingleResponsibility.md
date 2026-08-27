# Single Responsibility Principle (SRP)

## Definition

> A class should have **one reason to change**.

It does **not** mean a class should have only one method.

The idea is that a class should have **one cohesive responsibility**.

---

## Simple Example

### ❌ Bad Design

Suppose we have a `User` class:

```cpp
class User {
public:
    void saveToDatabase() {
        // save user
    }

    void sendEmail() {
        // send email
    }
};

The User class has two responsibilities:

Managing user-related data
Sending emails
Saving data to the database

So it has multiple reasons to change.

✅ Good Design

Separate the responsibilities:

class User {
    // User data and user-related behavior
};


class UserRepository {
public:
    void save(const User& user) {
        // save user to database
    }
};


class EmailService {
public:
    void sendEmail(const User& user) {
        // send email
    }
};