#ifndef SCREENS_H
#define SCREENS_H

#include "rot_encoder.h";

class Screen {
    public: 
    virtual ~Screen() = default;
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual void handleInput();
};    


#endif