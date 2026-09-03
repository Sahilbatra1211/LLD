#pragma once
#include<bits/stdc++.h>
using namespace std;

enum class Symbol{
    EMPTY,
    X,
    O
};

inline string symbolToString(Symbol symbol) {
    switch (symbol) {
        case Symbol::EMPTY: return "EMPTY";
        case Symbol::X:     return "X";
        case Symbol::O:     return "O";
    }

    return "";
}