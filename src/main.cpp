#undef FILE_READ
#undef FILE_WRITE

#include <SdFat.h>
#include <gpio_viewer.h>

#include "sdcard_manager.h"
#include "dht_sensor.h"
#include "rtc_module.h"
#include "oled_u8g2.h"
#include "rot_encoder.h"
#include <TFT_eSPI.h>
#include <FS.h>



SdFat sd;
String timeNow;
String temp;
String hum;
int tempC;
RotaryEncoderManager encoder(32, 33, 15);

GPIOViewer gpio_viewer;
TFT_eSPI tft = TFT_eSPI();



void onButtonClick() {
    Serial.println("Button clicked!");
}

void onEncoderChange() {
    int32_t delta = encoder.getDelta();
    if(delta != 0) {
        Serial.printf("Encoder: %d (Δ: %d)\n", encoder.getPosition(), delta);
    }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Init-ing SD card");

  setupRTC();
  setupOLED();
  setupDHT();
  encoder.begin();


  if (!sd.begin(5, SD_SCK_MHZ(4))) {
    Serial.println("Local SD init failed");
    return;
  } else { 
      Serial.println("SD init success");
  }

Serial.println("write test starting");

  if (writeFile("/test.txt", "hi")) {
    Serial.println("write success");
  } else {
    Serial.println("write failed");
  }
  readFile("/test.txt");



    encoder.setClickHandler(onButtonClick);
    encoder.setDoubleClickHandler([](){
        Serial.println("Double click detected!");
    });
    encoder.setLongPressHandler([](){
        Serial.println("Long press - resetting encoder");
        encoder.resetPosition();
    });

  //gpio_viewer.connectToWifi("Aussie Broadband 6525", "xazabaxawe");


    


 //gpio_viewer.begin();

  Serial.begin(115200);
  tft.init();
  tft.setRotation(0);

  tft.loadFont("/fonts/better_vcr.vlw");
  
  tft.fillScreen(TFT_RED);
  delay(2000);
  
  tft.fillScreen(TFT_GREEN);
  delay(2000);
  
  tft.fillScreen(TFT_BLUE);
  delay(2000);
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.println("Screen test complete");

  delay(2000);
}

void showTime() {
  timeNow = getTime();
  printLineOLED(timeNow, 1, 50, 1);
  Serial.println(timeNow);




}

void dht() {
  temp = getTemperature();
  hum = getHumidity();
  printLineOLED("Temp:", 0, 10, 2);
  printLineOLED(temp, 64, 10, 1);
  printLineOLED("Hum:", 0, 30, 2);
  printLineOLED(hum, 64, 30, 1);
}


void loop() {
  dht();
    encoder.update();
    onEncoderChange(); 



  
    delay(1); 


    tft.setTextSize(2);
    tft.drawPixel(1, 1, TFT_BLUE);
    tft.drawPixel(319, 239, TFT_RED);
}


