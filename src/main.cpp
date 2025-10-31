#undef FILE_READ
#undef FILE_WRITE

#include <SdFat.h>
#include <gpio_viewer.h>

#include "sdcard_manager.h"
#include "dht_sensor.h"
#include "rtc_module.h"
#include "oled_u8g2.h"
#include "rot_encoder.h"



SdFat sd;
String timeNow;
String temp;
String hum;
int tempC;
RotaryEncoderManager encoder(32, 33, 15);

GPIOViewer gpio_viewer;



void onButtonClick() {
    Serial.println("Button clicked");
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
  }

  Serial.println("SD init success");

  if (writeFile("/test.txt", "hi")) {
    Serial.println("write success");
  } else {
    Serial.println("write failed");
  }
  readFile("/test.txt");


    // Set button handlers
    encoder.setClickHandler(onButtonClick);
    encoder.setDoubleClickHandler([](){
        Serial.println("Double click detected!");
    });
    encoder.setLongPressHandler([](){
        Serial.println("Long press - resetting encoder");
        encoder.resetPosition();
    });

  gpio_viewer.connectToWifi("Aussie Broadband 6525", "xazabaxawe");





 gpio_viewer.begin();
}

void showTime() {
  timeNow = getTime();
  printLineOLED(timeNow, 1, 50, 1);




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
  showTime();
    encoder.update(); // Handles both encoder and button
    onEncoderChange(); // Check for encoder changes
    
    // Other application code here
    delay(1); // Prevent watchdog timer issues
}


