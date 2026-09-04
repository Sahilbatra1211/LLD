#include "Consumer.h"
#include "Message.h"
#include "MessageQueue.h"
#include "Producer.h"
#include <bits/stdc++.h>
#include <thread>

using namespace std;

int main(){

    MessageQueue* messageQueue=new MessageQueue();
    Consumer* consumer1= new Consumer(messageQueue);
    Consumer* consumer2=new Consumer(messageQueue);
    Producer* producer= new Producer(messageQueue);
    Message* message=new Message(1,"Hey hello 1");
    
    Message* message2=new Message(2,"Hey hello 2");
    producer->produce(message);
    producer->produce(message2);

    thread t1(&Consumer::consume,consumer1);
    thread t2(&Consumer::consume,consumer2);

    producer->produce(message);

    t1.join();
    t2.join();
}