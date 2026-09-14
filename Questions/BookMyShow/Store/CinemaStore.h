#pragma once

#include <bits/stdc++.h>
#include "../Entity/CinemaHall.h"
using namespace std;

class CinemaStore {
    unordered_map<CinemaHall*,bool> halls;
public:
    void save(CinemaHall* hall){
        // we can also make index of city halls here
        halls[hall]=true;
    }
};