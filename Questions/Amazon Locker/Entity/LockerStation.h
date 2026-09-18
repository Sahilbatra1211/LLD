#pragma once

#include "Address.h"
#include "Locker.h"
#include <bits/stdc++.h>

using namespace std;

class LockerStation {
public:
    int id;
    Address* adress;
    vector<Locker*> lockers;
    
};