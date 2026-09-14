#pragma once

#include <bits/stdc++.h>
#include "../Store/ShowStore.h"
using namespace std;

class SearchService {
    ShowStore* showStore;
public:
        vector<Show*> searchShowsForAMovie(Movie* movie, City city){
            return showStore->getShowsForAMovieInACity(movie, city);
        }
};