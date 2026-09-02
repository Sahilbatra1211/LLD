#pragma once

#include <bits/stdc++.h>

using namespace std;

class Light;
class LightState;
class OnState;
class OffState;


// State interface
class LightState {
public:
    virtual bool pressButton(Light* light) = 0;
    virtual ~LightState() = default;
};


// Context
class Light {
private:
    LightState* lightState;

public:
    Light(LightState* state) : lightState(state) {}

    bool pressButton() {
        return lightState->pressButton(this);
    }

    void setState(LightState* state) {
        this->lightState = state;
    }
};


// Concrete State
class OffState : public LightState {
public:
    bool pressButton(Light* light) override;
};


// Concrete State
class OnState : public LightState {
public:
    bool pressButton(Light* light) override;
};


// Implement AFTER both classes are known

bool OffState::pressButton(Light* light) {

    cout << "Light was off and now turning on\n";

    light->setState(new OnState());

    return true;
}


bool OnState::pressButton(Light* light) {

    cout << "Light was on and now turning off\n";

    light->setState(new OffState());

    return true;
}

int main(){
    
}