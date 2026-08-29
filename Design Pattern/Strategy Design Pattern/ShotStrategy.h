#pragma once

class ShotStrategy{
    public:
    virtual void playshot()=0;
    virtual ~ShotStrategy()=default;
};