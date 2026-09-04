#pragma once

#include <bits/stdc++.h>

using namespace std;

class Message {
    int id;
    string content;
    public:
    Message(int id,string content):id(id),content(content){

    }
    
    int getId() { return id; }
    string getContent() { return content; }
};