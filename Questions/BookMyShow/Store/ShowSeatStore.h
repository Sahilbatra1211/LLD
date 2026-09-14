#pragma once

#include <bits/stdc++.h>
#include "../Entity/ShowSeat.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/BookMyShow/Enum/ShowSeatStatus.h"
using namespace std;

class ShowSeatStore {
    vector<ShowSeat*> showSeats;
public:

    ShowSeatStatus getSeatStatus(Seat* seat, Show* show){
        return ShowSeatStatus::AVAILABLE;
    }
    
};