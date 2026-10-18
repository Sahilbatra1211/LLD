#pragma once

#include <bits/stdc++.h>
using namespace std;

class Job {
    int id;
    public:
    void excecute(){
        cout<<"Job "<<id<<" is executing"<<endl;
    }
};


class JobScheduler2 {
    priority_queue<pair<chrono::system_clock::time_point, Job*>> jobQueue;
    mutex mtx;
    condition_variable cv;

    public:
    
    void scheduleJob(Job* job, chrono::system_clock::time_point time){
        unique_lock<mutex> l(mtx);
        jobQueue.push({time, job});
        cv.notify_all();
    }

    void executionWorker(){
        while(true){
            unique_lock<mutex> l(mtx);
            cv.wait(l, [this](){return !jobQueue.empty();});
            auto [time, job] = jobQueue.top();
            if(time >  ){
                cv.wait_until(l, time);
                continue;
            }
            jobQueue.pop();
            l.unlock();
            job->excecute();
            
        }
    }

};