#include<bits/stdc++.h>
using namespace std;

int flag=0;
shared_mutex mtx;
// situations where there are multiple reads if lock normally then there is no use of threads because we 
// will need to synchronously read.
// instead better approach that we have is to have shared locks
void readFlag(){
    shared_lock<shared_mutex> l(mtx);
    // cout<<flag<<endl;
}

// so on write it will be strict lock but on reads thats not the case
void writeFlag(){
    lock_guard<shared_mutex> l(mtx);
    flag=!flag;
    // cout<<"Write"<<flag<<endl;
}


int main(){
    thread t1(readFlag);   
    thread t2(readFlag);
    thread t3(writeFlag);
    thread t4(readFlag);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}