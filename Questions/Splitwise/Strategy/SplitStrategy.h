#pragma once
#include "../Entity/User.h"
#include <bits/stdc++.h>

using namespace std;

class SplitStrategy {
public:
    virtual vector<pair<User*,int>>  calculate(vector<User*> splitBetween,int amoun)=0;  
};
