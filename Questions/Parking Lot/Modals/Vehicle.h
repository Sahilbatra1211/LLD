#pragma once

#include "VehicleType.h"
#include <bits/stdc++.h>

using namespace std;

class Vehicle {

    VehicleType vehicleType;
    string number;

    public:
    Vehicle(string number,VehicleType vehicleType){
        this->number=number;
        this->vehicleType=vehicleType;
    }
};