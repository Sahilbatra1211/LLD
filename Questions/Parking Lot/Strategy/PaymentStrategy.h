#pragma once

#include <bits/stdc++.h>

using namespace std;

class PaymentStrategy {
public:
    virtual int calculate(int startTime,int endTIme)=0;
};