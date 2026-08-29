#include<bits/stdc++.h>
using namespace std;

// lets now finish the patterns
// observer pattern me i know there will be a subscriber and a publisher. Publisher is the one who
// publishes the message. Now We n
// toh obviously publisher will add subscribers to some vectors of its and subscriber should have a add function to get itself added
// now for one publisher i can think of the design there is add subscriber function and there is a publish function
// in subscriber there should be observer function.
// there can be multiple publishers as well.
// lets go
class ISubscriber{
    public:
    // observer will push
    virtual void observe(string message)=0;
};

class IPublisher{
public:
    virtual void addSubscriber(ISubscriber *subscriber)=0;
    virtual void removeSubscriber(ISubscriber *subscriber)=0;
    virtual void publish(string message)=0;
};



class Publisher1: public IPublisher{
    unordered_map<ISubscriber*,bool> subscribers;

    public:
    
    void addSubscriber(ISubscriber *subscriber){
        subscribers[subscriber]=true;
    }

    void removeSubscriber(ISubscriber *subscriber){
        if(subscribers.count(subscriber)){
            subscribers[subscriber]=false;
        }
    }

    void publish(string message){
        for(auto node: subscribers){
            if(node.second){
                node.first->observe(message);
            }
        }
    }

};

class Subscriber1: public ISubscriber{
    public:
    void observe(string message){
        cout<<message<<endl;
    }
};


int main(){
    cout<<"check"<<endl;
    Publisher1 *p1=new Publisher1();
    Subscriber1 *s1=new Subscriber1();

    p1->addSubscriber(s1);
    p1->publish("my first message");

}