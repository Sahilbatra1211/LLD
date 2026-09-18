#pragma once

#include "Customer.h"
#include "Locker.h"
#include "Package.h"
#include "../Enum/AssignmentStatus.h"
#include <bits/stdc++.h>

using namespace std;

class LockerAssignment {
public:
    Package* package;
    Locker* locker;
    Customer* customer;
    int pin;
    AssignmentStatus assignmentStatus;
    int expiry;
};