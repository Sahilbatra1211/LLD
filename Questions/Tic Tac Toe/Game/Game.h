#pragma once

#include <bits/stdc++.h>
#include "../Model/Board.h"
#include "../Model/Player.h"
using namespace std;


// okay again first lets discuss the functions that will come over here
// start function is one i can think of for sure.
// what else i dont remember
// turn
class Game {
    Player* player1;
    Player *player2;
    Board *board;
    Player *currTurn;

public:
    Game(Board *board, Player *player1, Player *player2){
        this->board=board;
        this->player1=player1;
        this->player2=player2;
        this->currTurn=player1;
    }

    void start(){
        cout<< "Tic Tac Toe Game has begun be ready!!"<<endl;

        while(1){
            board->displayBoard();
            
            cout<<"First turn is of:"<<currTurn->getName()<<endl;
            pair<int,int> c=currTurn->getCoordinates();
            bool res=board->makeAMove(c.first, c.second, currTurn->getSymbol());
            
            if(!res){
                cout<<"You made a wrong move and Game finished"<<endl;
            }

            if(board->isWin()){
                cout<<currTurn->getName()<<" Wins"<<endl;
                break;
            }

            if(board->isDraw()){
                cout<<"Its a Draw"<<endl;
            }

            switchTurn();
        }
        
    }

    void switchTurn(){
        if(currTurn==player1)currTurn=player2;
        else currTurn=player1;
    }
};