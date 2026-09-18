#pragma once

#include <bits/stdc++.h>
#include "../Entity/Package.h"
#include "../Entity/LockerStation.h"
#include "../Service/PackageHandlingService.h"
using namespace std;

// now we need to do package assignment
// package pickup
// both these should be done by package handling service
// this is just controller or orchestrator class
class LockerApp {
    PackageHandlingService* packageHandlingService;
public:
// we will have few functions

    void assignPackage(Package* package,LockerStation* lockerStation){
        packageHandlingService->assignPackage(package, lockerStation);
    }

    void pickUpPackage(Customer* customer, Package* package){


    }

    void removeExpiredPackages(){

    }
    
};