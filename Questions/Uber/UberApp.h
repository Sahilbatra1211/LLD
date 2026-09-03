#pragma once
#include "Entity/Driver.h"
#include "Entity/Location.h"
#include "Entity/Ride.h"
#include "Entity/Rider.h"
#include "Service/DriverAssignmentService.h"
#include "Service/FareService.h"
#include "Service/PaymentService.h"
#include "Service/RideService.h"
#include <bits/stdc++.h>
#include <stdexcept>

using namespace std;

class UberApp {
    DriverAssignmentService* driverAssignmentService;
    RideService *rideService;
    FareService *fareService;
    PaymentService *paymentService;


public:
    
    Ride* bookACab(Location *src, Location *dest, Rider* rider){
        // now what we need to do is find a driver 
        Driver *driver= driverAssignmentService->findDriver(src,dest);

        // now we need to create a ride
        Ride* ride=rideService->createNewRide(src, dest, driver, rider);

        return ride;
    }

    bool startJourney(Driver* driver,Ride *ride){
        
        // validate driver
        if(!rideService->validateDriver(driver, ride)){
            throw runtime_error("Wrong driver passed!!");
        }

        // now we need to change the state
        rideService->startJourney(ride);

        return true;
    }

    bool completeJourney(Ride* ride, Driver *driver){
        
        if(!rideService->validateDriver(driver, ride)){
            throw runtime_error("Wrong driver passed!!");
        }

        // now we need to change the state
        rideService->completeJourney(ride);  

        int expectedFare= fareService->calculateFare(ride);

        rideService->updateFare(ride);

        paymentService->initiatePayment();

        return true;
    }


};