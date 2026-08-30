#include<bits/stdc++.h>
using namespace std;

// okay i remember returning the object like in simple factory. So we pass the string and it returns
// the object
// like if i pass windows tool kit it will return me that object and if apple then that.
// i dont remember what else was there in factory.


class Button{
    public:
    virtual void pressButton()=0;
};

class WindowsButton:public Button{
    public:
    void pressButton(){
        cout<<"windows button pressed"<<endl;
    }
};

class MacButton: public Button{
    public:
    void pressButton(){
        cout<<"mac button pressed"<<endl;
    }
};

// simple factory
class ButtonFactory{
    public:
    static Button* createButton(string type){
        if(type=="mac"){
            return new MacButton();
        }
        else {
            return new WindowsButton();
        }
    }
};


// now lets understand factory method


int main(){
    cout<<"check"<<endl;
    Button *newButton= ButtonFactory::createButton("mac");
    newButton->pressButton();
}