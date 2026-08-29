#include<bits/stdc++.h>
using namespace std;

// now lets recall singleton pattern
// so i remember that each class should be intialised only once
// and one of the way to do is that using a private constructor
// and a static variable
// if its a private constructor how will we even call it once as well?
// there is a public function called instantiate or something and it checks if the static variable is null 
// or not and based on that it then intialise the object
// but there are lot of confusions I have over here.
// one is this static variable thing why it was even needed maybe because it can be accessed outside like
// in a static function you know that if its intialised or not
// but why not a normal memeber variable that also we can know butt statick variable will be sae for all objets thats why
// we need to have a static variable
// now questions is why do we need a private constructor so that no one can call the constructor and it can be empty
// then comes concept of locking where we need to put a lock so that two objects asynchronously cant be declared right.
// now we have implemented basic, now use some locks and all to not allow concurrency issues.
// i remember there is a mutex mtx .
// and there is unique_lock concept.
// now what if mutex are not allowed there is something called double check or something
// like maybe two ifs or something
class Singleton{
    static Singleton* obj;
    static mutex mtx;

    Singleton(){

    }
    
    public:
    static Singleton* getInstance(){
        unique_lock<mutex> lock(mtx);

        if(obj==NULL){
            obj=new Singleton();
            cout<<"returned new object"<<endl;
            return obj;
        }
        else {
            cout<<"returned same object"<<endl;
            return obj;
        }
    }
    
};

Singleton* Singleton:: obj=NULL;
mutex Singleton::mtx;

int main(){
    cout<<"check "<<endl;
    Singleton* instance=Singleton::getInstance();
    Singleton* instance2= Singleton::getInstance();
}