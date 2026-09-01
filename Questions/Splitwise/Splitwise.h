#pragma once

#include "Entity/Group.h"
#include "Entity/User.h"
#include "Service/BalanceService.h"
#include "Service/TransactionService.h"
#include "Strategy/SplitStrategy.h"
#include <bits/stdc++.h>
using namespace std;

class Splitwise {
    BalanceService *balanceService;
    TransactionService *transactionService;
    
public:
    bool addSplit(User* whoPaid, vector<User*> splitBetween,int amount, Group *group, SplitStrategy *splitStrategy){
        
        // first we need to add it to transaciton
        transactionService->addTransaction();

        // second step is to add balance
        balanceService->addBalance(whoPaid, splitBetween,amount,group,splitStrategy);

        return true;
    }

    bool settleDebt(User* user1, User* user2, int amount){
        return true;
    }

};