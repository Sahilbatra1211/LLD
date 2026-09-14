#pragma once

#include <bits/stdc++.h>
#include "../Entity/CinemaHall.h"
#include "../Store/CinemaStore.h"
#include "../Store/ShowStore.h"
#include "../Entity/Show.h"
using namespace std;

class AdminService {
    CinemaStore *cinemaStore;
    ShowStore *showStore;
public:
    void addCinemaHall(CinemaHall* hall){
        cinemaStore->save(hall);
    } 
    
    void addShowsToAHall(CinemaHall* hall,vector<Show*> shows){
        showStore->saveShowsToCinema(hall,shows);
    }
};