#pragma once

#include "VehicleType.h"
#include <bits/stdc++.h>

using namespace std;

class ParkingSlot {
    int id;
    VehicleType vehicleType;
    bool occupied;

    public:
    ParkingSlot(int id,VehicleType vehicleType,bool occupied):id(id),vehicleType(vehicleType),occupied(occupied){

    }

    bool getOccupied(){
        return occupied;
    }

    void setOccupied(bool occupied){
        this->occupied=occupied;
    }

};