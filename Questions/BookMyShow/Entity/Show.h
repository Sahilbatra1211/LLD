#pragma once

#include "Auditorium.h"
#include "CinemaHall.h"
#include "Movie.h"
#include <bits/stdc++.h>

using namespace std;

class Show {
    

public:
    Movie* movie;
    CinemaHall* hall;
    Auditorium* auditorium;
    int startTime;
    int duration;
    
};