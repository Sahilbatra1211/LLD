#pragma once

#include <bits/stdc++.h>
#include <vector>
#include "../enum/Symbol.h"
using namespace std;

// okay so I havent thought thoroughly funcitons of each class
// lets ee what funcitons are required in this class
// so game class will call this and will obviously 
// make a move which will contain coordinates of the board , this will validate first and then 
// return bool if move was successul or not
// validate funciton is obviosuly requried validate if a move can be made or not
// then we have hasWon() which returns true or false
// then we have isDraw() which checks if whole board is filled without anyone winning.
class Board {
    vector<vector<Symbol>> board;
    int n;
    int m;

    public:
    Board(int n, int m){
        vector<vector<Symbol>> tempBoard(n,vector<Symbol>(m,Symbol::EMPTY));
        board=tempBoard;
        this->n=n;
        this->m=m;
    }

    // board needs to have three functions
    bool makeAMove(int x, int y, Symbol s){
        
        if(!validate(x,y)){
            return false;
        }

        // now we just need to make the move
        board[x][y]=s;

        return true;
    }

    // it checks if a given cell is within boundaries and also valid
    bool validate(int x,int y){
        if(x<n && x>=0 && y<m && y>=0){
            if(board[x][y]==Symbol::EMPTY){
                return true;
            }
            else{
                return false;
            }
        }
        else{
            return false;
        }
    }

    // A line wins only if every cell is the same *and* not EMPTY.
    // EMPTY == EMPTY would otherwise count as a win on unused rows/cols.
    bool isWin(){
        // horizontal
        for(int i=0;i<n;i++){
            if(board[i][0]==Symbol::EMPTY) continue;
            bool flag=true;
            for(int j=1;j<m;j++){
                if(board[i][j]!=board[i][0]) flag=false;
            }
            if(flag) return true;
        }

        // vertical
        for(int j=0;j<m;j++){
            if(board[0][j]==Symbol::EMPTY) continue;
            bool flag=true;
            for(int i=1;i<n;i++){
                if(board[i][j]!=board[0][j]) flag=false;
            }
            if(flag) return true;
        }

        // main diagonal (square boards)
        if(n==m && board[0][0]!=Symbol::EMPTY){
            bool flag=true;
            for(int i=1;i<n;i++){
                if(board[i][i]!=board[0][0]) flag=false;
            }
            if(flag) return true;
        }

        // anti-diagonal
        if(n==m && board[0][n-1]!=Symbol::EMPTY){
            bool flag=true;
            for(int i=1;i<n;i++){
                if(board[i][n-1-i]!=board[0][n-1]) flag=false;
            }
            if(flag) return true;
        }

        return false;
    }

    bool isDraw(){
        if(!isWin()){
            for(int i=0;i<board.size();i++){
                for(int j=0;j<board[0].size();j++){
                    if(board[i][j]==Symbol::EMPTY)return false;
                }
            }
            return true;
        }
        else{
            cout<<"Player has already won";
            return false;
        }
    }

    void displayBoard(){
        for(int i=0;i<board.size();i++){
            for(int j=0;j<board[0].size();j++){
                cout<<symbolToString(board[i][j])<<" ";
            }
            cout<<endl;
        }
    }
};