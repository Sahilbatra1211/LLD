#pragma once
#include "../Enum/PackageSize.h"
#include "Customer.h"
#include <bits/stdc++.h>

using namespace std;

class Package {
public:
    int id;
    string name;
    Customer* customer;
    PackageSize packageSize;
};