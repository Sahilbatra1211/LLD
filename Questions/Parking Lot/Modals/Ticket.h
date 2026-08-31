#pragma once

#include "ParkingSlot.h"
#include "Vehicle.h"
#include <bits/stdc++.h>

using namespace std;

class Ticket {
    int id;
    
    ParkingSlot* parkingSlot;
    int entryTime;
    int exitTIme;
    public:

    public:
    Vehicle *vehicle;
    Ticket(Vehicle* vehicle,ParkingSlot *parkingSlot, int entryTime):vehicle(vehicle),
    parkingSlot(parkingSlot),
    entryTime(entryTime){

    }
    
};