#pragma once

#include <bits/stdc++.h>
#include "../Entity/Ride.h"
using namespace std;

class FareCalculationStrategy {
public:
    virtual int calculateFare(Ride* ride)=0;
    
};