# Open/Closed Principle (OCP)

## Definition

> Software entities should be **open for extension but closed for modification**.

In simple terms:

> We should be able to **add new behavior without modifying existing, stable code**.

---

## Simple Example

### ❌ Bad Design

Suppose we have a payment processor:

```cpp
class PaymentProcessor {
public:
    void pay(string type, double amount) {
        if (type == "CARD") {
            // Card payment
        }
        else if (type == "UPI") {
            // UPI payment
        }
    }
};

Now suppose we want to add PayPal:

Add PayPal
    ↓
Modify PaymentProcessor

If we keep adding payment methods, we keep modifying the same class.

This violates OCP.

✅ Good Design

Create an abstraction:

class PaymentMethod {
public:
    virtual void pay(double amount) = 0;
    virtual ~PaymentMethod() = default;
};

Create different implementations:

class CardPayment : public PaymentMethod {
public:
    void pay(double amount) override {
        // Card payment
    }
};

class UPIPayment : public PaymentMethod {
public:
    void pay(double amount) override {
        // UPI payment
    }
};

The processor uses the abstraction:

class PaymentProcessor {
private:
    PaymentMethod& paymentMethod;

public:
    PaymentProcessor(PaymentMethod& paymentMethod)
        : paymentMethod(paymentMethod) {}

    void process(double amount) {
        paymentMethod.pay(amount);
    }
};

Now, to add PayPal:

class PayPalPayment : public PaymentMethod {
public:
    void pay(double amount) override {
        // PayPal payment
    }
};

We simply add a new class.

We don't need to modify PaymentProcessor.