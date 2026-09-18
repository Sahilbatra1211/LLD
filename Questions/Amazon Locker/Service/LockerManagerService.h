#pragma once

#include <bits/stdc++.h>
#include "../Entity/Locker.h"
#include "../Enum/PackageSize.h"
#include "../Entity/LockerStation.h"
#include "../Strategy/LockerFindingStrategy.h"
using namespace std;

class LockerManagerService {
    LockerFindingStrategy* lockerFindingStrategy;
public:

    Locker* findLocker(PackageSize packageSize,LockerStation* lockerStation){
        // we match the locker
        return new Locker();
    }
    
};