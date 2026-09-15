#pragma once

#include "AtmState.h"

class AuthenticatedState : public AtmState {
public:
    void insertCard(ATMMachine* atm, Card* card) {
    }

    void enterPin(ATMMachine* atm, int pin) {
    }

    void withdrawCash(ATMMachine* atm, int amount) {
    }

    void depositCash(ATMMachine* atm, int amount) {
    }

    void ejectCard(ATMMachine* atm) {
    }
};
