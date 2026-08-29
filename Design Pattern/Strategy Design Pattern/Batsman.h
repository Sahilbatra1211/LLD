#pragma once

#include "ShotStrategy.h"

class Batsman{
    ShotStrategy *shotStrategy;

    public:
    Batsman(ShotStrategy *shotStrategy);

    void setShotStrategy(ShotStrategy *shotStrategy);

    void playshot();
};