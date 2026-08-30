#include<bits/stdc++.h>
#include <condition_variable>
#include <mutex>
using namespace std;

// ill make a message queue and will have a producer and a consumer and
// what ill do is use condition variable 
// so without condiiton variable consumer thread will keep looking for the message
// since consumer and producer share same lock . it will be deadlock as producer will never be able to
// acquire the lock
// one thing I am thinking is that one ill have a producer class and other 
// ill have a consumer class 
// now whole point is where do we store the message 
// if we see the class diagram producer class it has a message quure class and
// consumer class which also has message queue class.
// they share the same class and now both of them call the funciton
// now how come they will share the lock
// thread will be made of class or what like object will push
// and now on another thread another object will pull
// now two different threads two different classes how will they share condition variable
// lets first design the producer consumer and then see hows the desing and 
class MessageQueue{
    queue<string> q;
    mutex mtx;
    condition_variable cv;

    public:
    void push(string message){
        lock_guard<mutex> l(mtx);
        q.push(message);
        cv.notify_one();
    }

    bool isEmpty(){
        return q.empty();
    }

    string pull(){
        unique_lock<mutex> l(mtx);
        cv.wait(l,[this]{return !q.empty();});
        string t=q.front();
        q.pop();
        return t;
    }
};

class Publisher{
    MessageQueue *messageQueue;

    public:
    Publisher(MessageQueue *messageQueue){
        this->messageQueue=messageQueue;
    }

    void publish(string message){
        messageQueue->push(message);
    }  
};

class Subscriber{
    MessageQueue *messageQueue;

    public:
    Subscriber(MessageQueue *messageQueue){
        this->messageQueue=messageQueue;
    }
    void pullMessage(){
        while(1){
            cout<<messageQueue->pull();
        }
    }
};

int main(){

    MessageQueue *messageQueue=new MessageQueue();
    Publisher *publisher = new Publisher(messageQueue);
    Subscriber *subscriber= new Subscriber(messageQueue);

    thread t1([&]{publisher->publish("some message");});
    thread t2([&]{
        subscriber->pullMessage();
        ;});
        t1.join();
        t2.join();
        thread t3([&]{publisher->publish("some message 2");});
        thread t4([&]{
            subscriber->pullMessage();
            ;});
            t3.join();
            t4.join();
}