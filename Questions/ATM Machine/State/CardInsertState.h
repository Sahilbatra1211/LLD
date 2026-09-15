#pragma once

#include "AtmState.h"
#include "../Service/AuthenticationService.h"
#include "AuthenticatedState.h"
#include <stdexcept>

class CardInsertState : public AtmState {
    AuthenticationService* authenticationService;

public:
    void insertCard(ATMMachine* atm, Card* card) {
    }

    void enterPin(ATMMachine* atm, int pin);

    void withdrawCash(ATMMachine* atm, int amount) {
    }

    void depositCash(ATMMachine* atm, int amount) {
    }

    void ejectCard(ATMMachine* atm) {
    }
};

#include "../Orchestrator/ATMMachine.h"

inline void CardInsertState::enterPin(ATMMachine* atm,int pin){
    if(authenticationService->checkPin(new Card(), pin)){
        cout<<"pin was correct";
        atm->setState(new AuthenticatedState());
    }
    else{
        throw runtime_error("Wrong pin");
    }
}
