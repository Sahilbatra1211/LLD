#pragma once

#include "Message.h"
#include <bits/stdc++.h>
#include <condition_variable>
#include <mutex>

using namespace std;

// now we need to apply locks and all to handle concurrency
// so when producer is pushing we need a lock
// and when someone is pulling then we need a lock 
// and producer should notify pull message that we have pushed something 
class MessageQueue {
    queue<Message*> q;
    mutex mtx;
    condition_variable cv;

public:
    bool pushMessage(Message* message){
        lock_guard<mutex> l(mtx);
        q.push(message); 
        cv.notify_one();
        return true;   
    }
    
    Message* pullMessage(){
        unique_lock<mutex> l(mtx);
        cv.wait(l, [this] { return !q.empty(); });
        Message* m=q.front();
        q.pop();
        return m;
    }
};