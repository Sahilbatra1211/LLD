#pragma once

#include <bits/stdc++.h>
using namespace std;

class PlayerStrategy {
public:
    virtual pair<int,int> giveCoordinates()=0;
};