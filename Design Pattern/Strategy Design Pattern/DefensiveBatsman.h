#pragma once
#include "ShotStrategy.h"
#include "Batsman.h"

class DefensiveBatsman: public Batsman{
    public:
    DefensiveBatsman(ShotStrategy *shotStrategy): Batsman(shotStrategy){

    }
};