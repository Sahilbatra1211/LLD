#pragma once

#include <bits/stdc++.h>
#include "../Strategy/FareCalculationStrategy.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/Uber/Entity/Ride.h"
using namespace std;

class FareService {
    FareCalculationStrategy* fareCalculationStrategy;
public:

    int calculateFare(Ride* ride){
        // right now not implementing it
        return 1000;
    }
    
};