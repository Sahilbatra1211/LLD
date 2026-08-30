#pragma once

#include "PlayerStrategy.h"
#include <bits/stdc++.h>

using namespace std;

class HumanPlayerStrategy: public PlayerStrategy {
public:
    pair<int,int> giveCoordinates(){
        cout<<"Give your coordinates x and y"<<endl;
        int x,y;
        cin>>x>>y;
        return {x,y};
    }
};