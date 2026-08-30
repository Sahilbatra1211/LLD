#include<bits/stdc++.h>
#include<thread>
using namespace std;

void helloWorld(){
    cout<<this_thread::get_id()<<endl;
    cout<<"Hello World"<<endl;
}

int main(){
    cout<<this_thread::get_id()<<endl;

    thread t(helloWorld);
    t.join();

    vector<thread> t2;

    for(int i=0;i<5;i++){
        t2.push_back(thread(helloWorld));
    }

    for(int i=0;i<5;i++){
        t2[i].join();
    }
}

