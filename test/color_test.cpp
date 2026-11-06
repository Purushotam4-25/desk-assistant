#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

struct Mode {
    const char* name;
    bool bgr;
    bool inverted;
};

const Mode modes[] = {
    {"1: BGR  inv OFF", true, false},
    {"2: RGB  inv OFF", false, false},
    {"3: BGR  inv ON", true, true},
    {"4: RGB  inv ON", false, true},
};

const uint8_t MADCTL = 0x36;
const uint8_t MX = 0x40;
const uint8_t BGR_BIT = 0x08;

struct Swatch {
    const char* name;
    uint16_t color;
};

const Swatch swatches[] = {
    {"RED", 0xF800},
    {"GREEN", 0x07E0},
    {"BLUE", 0x001F},
    {"WHITE", 0xFFFF},
    {"ORANGE", 0xFCC0},
};

void showMode(const Mode& mode) {
    tft.writecommand(MADCTL);
    tft.writedata(MX | (mode.bgr ? BGR_BIT : 0));
    tft.invertDisplay(mode.inverted);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString(mode.name, 6, 8);

    for (int i = 0; i < 5; i++) {
        int y = 36 + i * 41;
        tft.fillRect(0, y, 320, 41, swatches[i].color);
        tft.fillRect(6, y + 10, 96, 22, TFT_BLACK);
        tft.drawString(swatches[i].name, 10, y + 14);
    }
}

void setup() {
    Serial.begin(115200);
    tft.init();
    tft.setRotation(0);
}

void loop() {
    static uint8_t index = 0;
    static uint32_t lastChange = 0;

    if (lastChange == 0 || millis() - lastChange > 5000) {
        lastChange = millis();
        showMode(modes[index]);
        Serial.println(modes[index].name);
        index = (index + 1) % 4;
    }
}
