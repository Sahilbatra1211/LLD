#pragma once

class AtmState;
class Card;
class IdleState;

class ATMMachine {
    AtmState* atmState;
    Card* card;
public:
    ATMMachine();

    void setState(AtmState* state) {
        atmState = state;
    }

    void setCard() {
    }

    void eraseCard() {
    }

    void insertCard() {
    }

    void enterPin() {
    }

    void withdrawCash() {
    }

    void depositCash() {
    }

    void ejectCard() {
    }
};

#include "../State/IdleState.h"

inline ATMMachine::ATMMachine() {
    atmState = new IdleState();
}
