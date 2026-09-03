#pragma once

#include "Driver.h"
#include "Location.h"
#include "Rating.h"
#include "Rider.h"
#include "../State/RideState.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/Uber/Enum/PaymentStatus.h"
#include "../State/DriverWaiting.h"
#include <bits/stdc++.h>

using namespace std;

class Ride {
    Location* src;
    Location* dest;
    Driver* driver;
    Rider* rider;
    Rating *rating;
    RideState *rideState;
    int fare;
    PaymentStatus payementStatus;

public:
    Ride(Location* src, Location *dest,Driver* driver, Rider* rider):src(src),
    dest(dest),
    driver(driver),
    rider(rider){
        rideState=new DriverWaiting();
    }

    void setState(RideState* rideState){
        rideState=rideState;
    }
};