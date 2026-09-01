#pragma once

#include "User.h"
#include <bits/stdc++.h>

using namespace std;

class Group {

    vector<User*> users;
    string groupName;
    int id;

    public:
    Group(vector<User*> users, string groupName, int id):users(users),
    groupName(groupName),
    id(id){

    }
    
};