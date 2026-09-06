#pragma once
#include "RateLimitingStrategy.h"
#include "../Store/FixedWindowStore.h"
#include <bits/stdc++.h>
using namespace std;

class FixedWindowStrategy: public RateLimitingStrategy {
    FixedWindowStore* requestStore;
public:
    FixedWindowStrategy(FixedWindowStore* requestStore){
        this->requestStore=requestStore;
    }

    bool isAllowed(int limit,int key,int time){
        int window=time/60;
        int currCount=requestStore->read(key,window);
        if(currCount<limit){
            currCount++;
            requestStore->save(key, window,currCount);
            return true;
        }
        return false;
    }
    
};