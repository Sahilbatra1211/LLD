#include<bits/stdc++.h>
#include <mutex>
#include <thread>
using namespace std;

class JobScheduler{
    queue<int> q;
    mutex mtx;
    bool shutDown; 
    condition_variable cv;

    public:

    void addJob(int x){
        unique_lock<mutex> l(mtx);
        q.push(x);
        l.unlock();
        cv.notify_one();
    }

    void worker(){
        while(1){
            unique_lock<mutex> l(mtx);
            cv.wait(l,[this]{return (!q.empty()|| shutDown);});

            if(q.empty() && shutDown){
                l.unlock();
                break;
            }
            cout<<this_thread::get_id()<<" "<<q.front()<<endl;
            q.pop();
            l.unlock();
            // and now task can be performed by the thread
        }
    }
    void shutdown(){
        unique_lock<mutex> l(mtx);
        shutDown=true;
        l.unlock();
        cv.notify_all();
        
    }
};



int main(){

        // now first we need to run the worker threads
        JobScheduler* jobScheduler=new JobScheduler();
        vector<thread> t;
        for(int i=0;i<5;i++){
            t.push_back(thread(&JobScheduler::worker,jobScheduler));
        }

        jobScheduler->addJob(1);
        jobScheduler->addJob(2);
        jobScheduler->addJob(3);
        jobScheduler->addJob(4);
        jobScheduler->addJob(5);
        jobScheduler->addJob(6);
        jobScheduler->addJob(7);
        jobScheduler->addJob(8);
        jobScheduler->addJob(9);
        
        jobScheduler->shutdown();
        for(int i=0;i<5;i++){
            t[i].join();
        }

}