#pragma once

#include "AtmState.h"
#include <stdexcept>

class IdleState : public AtmState {
public:
    void insertCard(ATMMachine* atm, Card* card);
    void enterPin(ATMMachine* atm, int pin);
    void withdrawCash(ATMMachine* atm, int amount);
    void depositCash(ATMMachine* atm, int amount);
    void ejectCard(ATMMachine* atm);
};

#include "../Orchestrator/ATMMachine.h"

inline void IdleState::insertCard(ATMMachine* atm, Card* card) {
    atm->setState(new IdleState());
}

inline void IdleState::enterPin(ATMMachine* atm, int pin) {
    throw std::runtime_error("card not inserted yet");
}

inline void IdleState::withdrawCash(ATMMachine* atm, int amount) {
    throw std::runtime_error("card not inserted yet");
}

inline void IdleState::depositCash(ATMMachine* atm, int amount) {
    throw std::runtime_error("card not inserted yet");
}

inline void IdleState::ejectCard(ATMMachine* atm) {
    throw std::runtime_error("card not inserted yet");
}
