#pragma once

#include "PaymentStrategy.h"
#include <bits/stdc++.h>

using namespace std;

class StandardPaymentStrategy: public PaymentStrategy {
public:
    int calculate(int startTime,int endTime){
        return (endTime-startTime)*10;
    }
};