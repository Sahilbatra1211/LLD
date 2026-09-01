#pragma once

#include "User.h"
#include <bits/stdc++.h>

using namespace std;

class Balance {
    User* user1;
    User* user2;
    int amount;
    
public:
    Balance(User* user1, User* user2, int amount): user1(user1),user2(user2),amount(amount){
        
    }
};