#pragma once

#include "Entity/CinemaHall.h"
#include "Entity/User.h"
#include "Service/AdminService.h"
#include "./Enum/City.h"
#include "Service/BookingService.h"
#include "Service/SearchService.h"
#include <bits/stdc++.h>

using namespace std;

class BookMyShow {
    AdminService* adminService;
    SearchService* searchService;
    BookingService* bookingService;
public:
    
    void addNewCinemaHall(CinemaHall* hall){
        adminService->addCinemaHall(hall);
    }

    void addShowsToHall(CinemaHall* hall,vector<Show*> shows){
        adminService->addShowsToAHall(hall, shows);
    }

    vector<Show*> searchShowsForAMovie(City city,Movie* movie){
        return searchService->searchShowsForAMovie(movie,city);
    }

    void bookAShow(Show* show, Seat* seat, User* user){
        bookingService->bookAShow(show, seat);
    }
};