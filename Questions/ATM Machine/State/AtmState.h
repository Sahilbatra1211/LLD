#pragma once

class ATMMachine;
class Card;

class AtmState {
public:
    virtual ~AtmState() = default;
    virtual void insertCard(ATMMachine* atm, Card* card) = 0;
    virtual void enterPin(ATMMachine* atm, int pin) = 0;
    virtual void withdrawCash(ATMMachine* atm, int amount) = 0;
    virtual void depositCash(ATMMachine* atm, int amount) = 0;
    virtual void ejectCard(ATMMachine* atm) = 0;
};
