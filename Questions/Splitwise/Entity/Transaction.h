#pragma once

#include "User.h"
#include "../Enums/TransactionType.h"
#include "../Strategy/SplitStrategy.h"
#include <bits/stdc++.h>

using namespace std;

class Transaction {
    User* whoPaid;
    vector<User*> splitBetween;
    int date;
    TransactionType transactionType;
    int amount;
    SplitStrategy *splitStrategy;

    public:
    Transaction(User* whoPaid, vector<User*> splitBetween, int date, TransactionType transactionType,
    int amount, SplitStrategy *splitStrategy): whoPaid(whoPaid),
    splitBetween(splitBetween),
    date(date),
    transactionType(transactionType),
    amount(amount),
    splitStrategy(splitStrategy){
        
    }
    
};