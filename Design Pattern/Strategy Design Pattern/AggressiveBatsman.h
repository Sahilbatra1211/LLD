#pragma once
#include "ShotStrategy.h"
#include "Batsman.h"

class AggresiveBatsman: public Batsman{
    public:
    AggresiveBatsman(ShotStrategy *shotStrategy): Batsman(shotStrategy){

    }
};