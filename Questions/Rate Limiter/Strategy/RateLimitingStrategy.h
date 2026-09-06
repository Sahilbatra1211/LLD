#pragma once
#include <bits/stdc++.h>

using namespace std;

class RateLimitingStrategy {


public:

    virtual bool isAllowed(int limit,int key,int time)=0;
    
};