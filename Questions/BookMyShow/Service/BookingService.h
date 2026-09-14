#pragma once

#include <bits/stdc++.h>
#include <mutex>
#include <stdexcept>
#include "../Entity/Seat.h"
#include "../Entity/Show.h"
#include "../Store/ShowSeatStore.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/BookMyShow/Enum/ShowSeatStatus.h"
using namespace std;

class BookingService {
    ShowSeatStore* showSeatStore;
    unordered_map<Show*,unordered_map<Seat*,mutex*>> mutexes;
    public:

    void bookAShow(Show* show, Seat* seat){
        // first check if this seat is available
        Auditorium* audi=show->auditorium;

        if(audi->checkSeat(seat)){
            if(!mutexes.count(show) && mutexes[show].count(seat)){
                mutexes[show][seat]=new mutex();
            }

            unique_lock<mutex> l(*mutexes[show][seat]);
            // now seat is available now we want to see if its alraedy booked
            if(showSeatStore->getSeatStatus(seat, show)==ShowSeatStatus::AVAILABLE){
                // we will create a new object and save it to our showSeatStore

            }
        }
        else{
            throw runtime_error("not a valid seat");
        }
    }
};