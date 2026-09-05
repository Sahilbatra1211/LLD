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

    void execute(){
        cout<<"execute a job"<<endl;
    }
};


class JobScheduler {
    queue<Job*> jobQueue;
    vector<thread> threads;
    mutex mtx;
    condition_variable cv;
    int shutDown;

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
                cv.wait(l,[this]{return (!jobQueue.empty()|| shutDown);});
                if(jobQueue.empty() && shutDown)break;
                Job* j=jobQueue.front();
                jobQueue.pop();
                l.unlock();     
                try{
                    j->execute();
                }
                catch(exception e){
                    // consume;
                } 
            }
        }

    void shutdown(){
        unique_lock<mutex> l(mtx);
        shutDown=1;
        cv.notify_all();
    }


    ~JobScheduler(){
        for(int i=0;i<threads.size();i++){
            threads[i].join();
        }
    }
    
};