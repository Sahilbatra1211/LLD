#pragma once
#include "../Entity/User.h"
#include "../Entity/Balance.h"
#include "../Entity/Group.h"
#include <bits/stdc++.h>
#include <stdexcept>

using namespace std;

class BalanceStore {
    unordered_map<User*,vector<Balance*>> globalBalances;
    unordered_map<Group*,vector<Balance*>> groupBalances;

public:
    bool storeBalances(vector<Balance*> balances, Group *group){
        throw runtime_error("not implemnted");
    }
};