#pragma once
#include "../Entity/Driver.h"
#include "../Entity/Location.h"
#include <bits/stdc++.h>

using namespace std;

class DriverAssignmentService {
public:

    Driver* findDriver(Location *src,Location* dest){
        // for now returning the driver as is
        return new Driver("sahil");
    }
    
};