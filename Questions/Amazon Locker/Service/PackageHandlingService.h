#pragma once
#include "../Entity/LockerStation.h"
#include "../Entity/Package.h"
#include "../Entity/Customer.h"
#include "LockerManagerService.h"
#include "../Strategy/LockerPinGeneratorStrategy.h"
#include <bits/stdc++.h>

using namespace std;

class PackageHandlingService {
    // okay all the thing related to lockers should be handled by lcoker service like finding a locker and all
    // marking a locker etc
    LockerManagerService* lockerManagerService;
    LockerPinGeneratorStrategy* lockerPinGeneratorStrategy;
public:

    void assignPackage(Package* package,LockerStation *station){
        // now lets look at assignment flow
        // first we need to find the locker so there cna be multiple strategies to do that
        Locker* locker=lockerManagerService->findLocker(package->packageSize, station);

        // now once we have the locker then we need to generate a pin
        // we will find a pin by lockerpin generator strtegy
        int pin =1;

        // now once we have a pin
        // now we want to do assingment object
        // we will create an object of lockerAssignment

        // then we will change the status of the locker using lockerManagerService
        // then we will notify the customer using notificaiton service
    }


    void pickUpPackage(Customer* customer, Package* package){


    }

    void removeExpiredPackages(){

    }
    
};