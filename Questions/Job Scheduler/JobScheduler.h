#pragma once
#include <bits/stdc++.h>
#include <mutex>

using namespace std;

class Job{
    int id;
    string name;

    public:

    Job(int id,string name):id(id),name(name){

    }
};


class JobScheduler {
    queue<Job*> jobQueue;
    vector<thread> threads;
    mutex mtx;
    condition_variable cv;

public:

    JobScheduler(int maxThreads){
        for(int i=0;i<maxThreads;i++){
            threads.push_back(thread(&JobScheduler::schedule, this));
        }
    }

    void addJob(Job* job){
        unique_lock<mutex> l(mtx);
        jobQueue.push(job);
        l.unlock();
        cv.notify_one();
    }

    void schedule(){
        while(1){
            unique_lock<mutex> l(mtx);
            cv.wait(l,[this]{return !jobQueue.empty();});
            Job* j=jobQueue.front();
            jobQueue.pop();
            cout<<"some job executed"<<endl;
            l.unlock();        
        }
    }


    ~JobScheduler(){
        for(int i=0;i<threads.size();i++){
            threads[i].join();
        }
    }
    
};