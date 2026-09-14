#pragma once

#include <bits/stdc++.h>
#include "../Entity/CinemaHall.h"
#include "../Enum/City.h"
#include "../Entity/Show.h"
using namespace std;

class ShowStore {
    unordered_map<CinemaHall*,vector<Show*>> showHallMapping;
public:

    void saveShowsToCinema(CinemaHall* hall, vector<Show*> shows){
        for(auto node: shows){
            showHallMapping[hall].push_back(node);
        }
    }

    vector<Show*> getShowsForAMovieInACity(Movie* movie,City city){
        vector<Show*> v;
        return v;
    }

};