#include<bits/stdc++.h>
#include "AggressiveBatsman.h"
#include "DefensiveBatsman.h"
#include "ShotStrategy.h"
#include "DriveShot.h"
using namespace std;

int main(){
   // cout<<"run basic program"<<endl;

    ShotStrategy *driveShotStrategy=new DriveShot();

    AggresiveBatsman *viratKohli= new AggresiveBatsman(driveShotStrategy);

    viratKohli->playshot();

}