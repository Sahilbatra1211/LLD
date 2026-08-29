#include<bits/stdc++.h>
#include "batsman.h"
using namespace std;

Batsman:: Batsman(ShotStrategy *shotStrategy){
    this->shotStrategy=shotStrategy;
}

void Batsman::setShotStrategy(ShotStrategy *shotStrategy){
    this->shotStrategy = shotStrategy;
}

void Batsman::playshot(){
    this->shotStrategy->playshot();
}

