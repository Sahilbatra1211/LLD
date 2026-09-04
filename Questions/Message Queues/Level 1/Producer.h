#pragma once

#include "Message.h"
#include "MessageQueue.h"
#include <bits/stdc++.h>

using namespace std;

class Producer {
    MessageQueue* messageQueue;
public:
    Producer(MessageQueue* messageQueue):messageQueue(messageQueue){

    }    

    bool produce(Message* message){
        return messageQueue->pushMessage(message);
    }
};