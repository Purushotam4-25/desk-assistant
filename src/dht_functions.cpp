#include <DHT.h>
#include "dht_sensor.h"


#define DHTPIN 4 
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


void setupDHT() {
  // put your setup code here, to run once:
  dht.begin();
}

float getTemperature(){
  return dht.readTemperature();
}

float getHumidity(){
  return dht.readHumidity();
  Serial.println(dht.readHumidity());
}