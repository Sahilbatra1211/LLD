#pragma once

#include "RequestStore.h"
#include <bits/stdc++.h>

using namespace std;

class FixedWindowStore:RequestStore {
public:
    void save(int key,int window,int value){
        requests[key][window]=value;
    }

    int read(int key,int window){
        return requests[key][window];
    }
    
};