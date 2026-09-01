#pragma once
#include "../Store/BalanceStore.h"
#include "../Strategy/SplitStrategy.h"
#include "../Entity/Balance.h"
#include "../Entity/User.h"
#include "../Entity/Group.h"
#include <bits/stdc++.h>

using namespace std;

class BalanceService {
    BalanceStore* balanceStore;
public:

    bool addBalance(User* whoPaid, vector<User*> splitBetween,int amount, Group *group, SplitStrategy *splitStrategy){
        // first we need to create a balance
        vector<pair<User*,int>> splits= splitStrategy->calculate(splitBetween,amount);

        // now we will make these splits as balances
        vector<Balance*> balances;
        for(auto node: splits){
            if(node.first!=whoPaid){
                balances.push_back(new Balance(whoPaid,node.first,node.second));
            }
        }

        balanceStore->storeBalances(balances,group);
        return true;
    }
    
};