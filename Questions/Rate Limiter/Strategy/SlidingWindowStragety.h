#pragma once
#include "RateLimitingStrategy.h"
#include "../Store/FixedWindowStore.h"
#include <bits/stdc++.h>
using namespace std;

class SlidingWindowStragety : public RateLimitingStrategy{
    FixedWindowStore* requestStore;
public:
    SlidingWindowStragety(FixedWindowStore* requestStore){
       this->requestStore=requestStore;
    }

    bool isAllowed(int limit,int key,int time){
        int window=time/60;
        int prevWindow=requestStore->read(key,window-1);
        int currWindow=requestStore->read(key,window);
        double percent=((double)time-(double)(60*window))/(double)60;
        double prevPercent=100-percent;
        int countFromPrevWindow=prevPercent*prevWindow/100;

        if(countFromPrevWindow+currWindow<limit){
            currWindow++;
            requestStore->save(key, window, currWindow);
            return true;
        }

        return false;    
    }
};