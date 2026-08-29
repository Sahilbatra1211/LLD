#pragma once
#include "ShotStrategy.h"

class PullShot: public ShotStrategy{
    public:
    void playshot(){
        cout<<"Play pull shot"<<endl;
    }
};