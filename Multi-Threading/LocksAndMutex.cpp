#include<bits/stdc++.h>
#include<thread>
#include<mutex>
using namespace std;

mutex mtx;

void helloWorld(){
    unique_lock<mutex> l(mtx);
    cout<<"Hello"<<this_thread::get_id()<<endl;
    l.unlock();
}

int main(){
    // now earlier we saw that when we were trying to output randomly output was coming.
    // we need to make sure one at a time a thread can access hello and print it
    // making it syncrhonous using locks  

    vector<thread> t;

    for(int i=0;i<5;i++){
        t.push_back(thread(helloWorld));
    }

    for(int i=0;i<5;i++){
        t[i].join();
    }
}