#include<bits/stdc++.h>
#include "./Model/Board.h"
#include "./Model/Player.h"
#include "./Game/Game.h"
#include "./enum/Symbol.h"
#include "Strategy/HumanPlayerStrategy.h"
#include "Strategy/PlayerStrategy.h"
using namespace std;

int main(){
    cout<<"Tic Toe Game"<<endl;
    PlayerStrategy *playerStrategy= new HumanPlayerStrategy();
    Player* player1=new Player("Sahil",Symbol::O,playerStrategy);
    Player* player2=new Player("Vishal", Symbol::X,playerStrategy);

    Board *newBoard= new Board(3,3);

    Game *newGame= new Game(newBoard,player1,player2);
    newGame->start();
    
}