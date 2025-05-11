#undef FILE_READ
#undef FILE_WRITE

#include <SdFat.h>
#include <gpio_viewer.h>

#include "sdcard_manager.h"
#include "dht_sensor.h"
#include "rtc_module.h"
#include "oled_u8g2.h"



SdFat sd;
String timeNow;
String temp;
String hum;
int tempC;
GPIOViewer gpio_viewer;





void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Init-ing SD card");

  setupRTC();
  setupOLED();
  setupDHT();


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
  delay(1000);




}


