#pragma once

#include <bits/stdc++.h>
#include "../Modals/Floor.h"
#include "../Modals/ParkingSlot.h"
using namespace std;

class SlotAvailableStrategy {
public:
    virtual ParkingSlot* findSlot(vector<Floor*> floors)=0;
};