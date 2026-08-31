#pragma once

#include "ParkingSlot.h"
#include <bits/stdc++.h>

using namespace std;

class Floor {
    vector<ParkingSlot*> parkingSlots;
public:
    vector<ParkingSlot*> getParkingSlots(){
        return parkingSlots;
    }
};