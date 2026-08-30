#pragma once
#include "../enum/Symbol.h"
#include "../Strategy/PlayerStrategy.h"
#include<bits/stdc++.h>
using namespace std;

// now we will introduce strategy
// there will be three types of strategies one is humanStrategy, RandomStrategy, AIStrategy
// so there is a has a relationship, strategy will be deifined in constructor and we can also change the strategy if required.
// now how will we use this strategy. In game when we start then in that game we will see that we will ask that specific player to 
// now that asking is based on this strategy so basically we will say player->moveStragetgy which will return the coordinates.
// now game will pass it to board
class Player{
    string playerName;
    Symbol symbol;
    PlayerStrategy *playerStrategy;

    public:
    Player(string playerName,Symbol symbol,PlayerStrategy *playerStrategy){
        this->playerName=playerName;
        this->symbol=symbol;
        this->playerStrategy=playerStrategy;
    }

    pair<int,int> getCoordinates(){
        return playerStrategy->giveCoordinates();
    }

    string getName(){
        return playerName;
    }

    Symbol getSymbol(){
        return symbol;
    }
};