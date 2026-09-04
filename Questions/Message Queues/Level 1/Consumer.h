#pragma once

#include "Message.h"
#include "MessageQueue.h"
#include <bits/stdc++.h>
#include <cstddef>

using namespace std;

class Consumer {
    MessageQueue* messageQueue;
public:

    Consumer(MessageQueue* messageQueue){
        this->messageQueue=messageQueue;
    }

    virtual Message* consume(){
        while(1){
            Message* message=messageQueue->pullMessage();
            if(message!=NULL){
                cout << "Consumed id=" << message->getId()
                     << " content=" << message->getContent() << endl;
                return message;
            }
        }
    }
};