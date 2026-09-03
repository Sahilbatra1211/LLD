#pragma once
#include "../Store/RideStore.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/Uber/Entity/Location.h"
#include "../Entity/Rider.h"
#include "../Entity/Driver.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/Uber/Entity/Ride.h"
#include "../State/RideInProgress.h"
#include "../State/RideCompleted.h"
#include <bits/stdc++.h>

using namespace std;

class RideService {
    RideStore* rideStore;
public:
    
    Ride* createNewRide(Location *src, Location *dest, Driver *driver, Rider *rider){
        // idealy we should save it in rider store but we are not implementing everything
        return new Ride(src,dest,driver,rider);
    }

    bool validateDriver(Driver *driver, Ride* ride){
        // for now 
        return true;
    }

    bool startJourney(Ride *ride){
        ride->setState(new RideInProgress());
        return true;
    }

    bool completeJourney(Ride* ride){
        ride->setState(new RideCompleted());
        return true;
    }

    void updateFare(Ride* ride){
        // fare updated
    }
};