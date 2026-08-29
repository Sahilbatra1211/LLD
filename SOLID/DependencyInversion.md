# Dependency Inversion Principle (DIP)

## Definition

> **High-level modules should not depend on low-level modules. Both should depend on abstractions.**

And:

> **Abstractions should not depend on details. Details should depend on abstractions.**

### Simple way to remember

> **"Depend on abstractions, not concrete implementations."**

---

## Simple Example

### ❌ Bad Design

Suppose our `UserService` directly uses MySQL:

```cpp
class MySQLDatabase {
public:
    void save() {
        // Save to MySQL
    }
};

class UserService {
private:
    MySQLDatabase database;

public:
    void createUser() {
        database.save();
    }
};

The dependency is:

UserService
     ↓
MySQLDatabase

UserService is tightly coupled to MySQL.

If we want to switch to PostgreSQL:

MySQL
  ↓
PostgreSQL

we need to modify UserService.

This violates DIP.

✅ Good Design

Create an abstraction:

class Database {
public:
    virtual void save() = 0;
    virtual ~Database() = default;
};

Concrete implementations depend on the abstraction:

class MySQLDatabase : public Database {
public:
    void save() override {
        // Save to MySQL
    }
};

class PostgreSQLDatabase : public Database {
public:
    void save() override {
        // Save to PostgreSQL
    }
};

Now UserService depends on the abstraction:

class UserService {
private:
    Database& database;

public:
    UserService(Database& database)
        : database(database) {}

    void createUser() {
        database.save();
    }
};

The dependency becomes:

          Database
          ↑      ↑
          |      |
       MySQL   PostgreSQL
          ↑
          |
     UserService

UserService doesn't care which database implementation is being used.

Dependency Injection

The following:

UserService(Database& database)
    : database(database) {}

is Dependency Injection.

The dependency is provided from outside instead of UserService creating it itself.

For example:

MySQLDatabase db;

UserService service(db);

We can easily switch:

PostgreSQLDatabase db;

UserService service(db);

The UserService code remains unchanged.

DIP vs Dependency Injection

They are related but not the same.

DIP

A design principle:

High-level code should depend on abstractions rather than concrete implementations.

Dependency Injection

A technique used to provide dependencies from outside.

DIP
 ↓
Depend on abstraction

Dependency Injection
 ↓
Pass the dependency from outside
Key Point

Without DIP:

High-level module
        ↓
Concrete implementation

With DIP:

High-level module
        ↓
    Abstraction
        ↑
Concrete implementation