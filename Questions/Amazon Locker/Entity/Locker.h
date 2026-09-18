#pragma once

#include <bits/stdc++.h>
#include "../Enum/LockerSize.h"
#include "../Enum/LockerStatus.h"
using namespace std;

class Locker {
public:
    int id;
    LockerSize lockerSize;
    LockerStatus* status;
};