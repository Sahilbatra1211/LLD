#pragma once
#include "RateLimitingStrategy.h"
#include "../Store/TokenBucketStore.h"
#include <bits/stdc++.h>
using namespace std;


// okay not in this algo there is a defined rate and we keep increaseing the bucket size per minute
// now what happens in this a request comes at a time and it can also serve burst of requests 
// we check if we have that limit right now for that user we get that limit and check
// if we have it then everything is fine, we reduce the limit but 
// okay so this max bucket is per user and now it will decrease for the given user and 
// ill need to increase for all the users right.
// so a funciton should be invoked every second or something and should increase the limit of all user and should run on a separate thread.
// so for this one users doesnt store the uses ones rather bucket size.
// now how does tokenBucketSt will look like it will for a given key will currLimitAvailableStored
class TokenBucketStrategy: public RateLimitingStrategy {
    int refillRate;
    int maxLimit;
    TokenBucketStore* requestStore;

public:
    TokenBucketStrategy(TokenBucketStore* requestStore,int refillRate,int maxLimit){
        this->requestStore=requestStore;
        this->refillRate=refillRate;
        this->maxLimit=maxLimit;
    }

    bool isAllowed(int limit,int key,int time){
        
        auto p=requestStore->getCurrLimitAndLastSynced(key);
        if(p.first==-1){
            // it means fst rrequest of user so
            requestStore->save(key,maxLimit-1,time);
            return true;
        }
        else{
            // first we need to update the tokens
            int newLimit=min(maxLimit,p.first+(time-p.second)*refillRate);
            requestStore->save(key, newLimit-1, time);
            return true;
        }
        return false;
    }
    

};