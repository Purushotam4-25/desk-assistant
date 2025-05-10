#ifndef RTC_H
#define RTC_H
#include <Arduino.h>
#include <RtcDS1302.h>




void setupRTC();
String getTime();
String getDate();
void setDateTime(int year, int month, int day, int hour, int min, int sec);



#endif