#pragma once

#include "RequestStore.h"
#include <bits/stdc++.h>

using namespace std;

class TokenBucketStore:RequestStore {
    // we need to store two things curr limit and second is lastSynced so we can evalute the updated token limit
    unordered_map<int,pair<int,int>> currLimit;
public:
    pair<int,int> getCurrLimitAndLastSynced(int key){
        if(currLimit.count(key)){
            return currLimit[key];
        }
        return {-1,-1};
    }

    void save(int key,int value,int time){
        currLimit[key]={value,time};
    }
};