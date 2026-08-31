#pragma once

#include "SlotAvailableStrategy.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/Parking Lot/Modals/Floor.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/Parking Lot/Modals/ParkingSlot.h"
#include <bits/stdc++.h>
#include <stdexcept>

using namespace std;

class NearestSlotAvailableStrategy: public SlotAvailableStrategy {
public:
    ParkingSlot* findSlot(vector<Floor*> floors){
       for(auto node: floors){
            for(auto node2: node->getParkingSlots()){
                if(!node2->getOccupied()){
                    return node2;
                }
            }
       }
       throw runtime_error("No parking slots available");
    }
};