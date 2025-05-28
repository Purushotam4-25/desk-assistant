#include "oled_u8g2.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, 22, 21);

void setupOLED() {
    oled.begin();
    oled.clearBuffer();
    oled.setFont(u8g2_font_ncenB08_tr);
    oled.sendBuffer();
    Serial.println("OLED Setup");
}

void printLineOLED(const String& text, int x, int y, int size) {
    oled.clearBuffer();

    switch (size){
        case 1:
            oled.setFont(u8g2_font_6x10_tr);
            break;
        case 2:
            oled.setFont(u8g2_font_ncenB08_tr);
            break;
        case 3:
            oled.setFont(u8g2_font_ncenB14_tr);
            break;
        default:
            oled.setFont(u8g2_font_6x10_tr);
            break;
        
    }

    oled.drawStr(x, y, text.c_str());
    oled.sendBuffer();
}

void clearOLED() {
    oled.clearBuffer();
    oled.sendBuffer(); 
}

