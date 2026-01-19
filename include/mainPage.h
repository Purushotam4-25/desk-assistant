#pragma once
#include "screens.h"
#include <TFT_eSPI.h>


class mainPage : public Screen {
    private:
        TFT_eSPI* tft;
        int activeSubpage = 0;

    public: 
        mainPage(TFT_eSPI* display) : tft(display){}
        void init() override;
        void update() override;
        void draw() override;
        void

};
