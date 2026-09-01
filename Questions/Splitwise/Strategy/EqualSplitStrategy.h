#pragma once

#include "SplitStrategy.h"
#include "c:/Users/sahil/OneDrive/Documents/LLD/Questions/Splitwise/Entity/User.h"
#include <bits/stdc++.h>
#include <stdexcept>

using namespace std;

class EqualSplitStrategy: public SplitStrategy {
public:
    vector<pair<User*,int>>  calculate( vector<User*> splitBetween, int amount){
        cout<<"Not implemented yet"<<endl;
        throw runtime_error("not implemented");
    }  
};