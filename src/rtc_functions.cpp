#include "rtc_module.h"
#include <RtcDS1302.h>
#include <Wire.h>
#include <ThreeWire.h>

#define RTC_CLK 27
#define RTC_DAT 26
#define RTC_RST 25


ThreeWire wire(RTC_DAT, RTC_CLK, RTC_RST);
RtcDS1302 <ThreeWire> rtc(wire);

void setupRTC() {
    rtc.Begin();

    if(!rtc.IsDateTimeValid()) {
        Serial.println("RTC has invalid DateTime. reset time");
        rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));

    }
    if (rtc.GetIsWriteProtected()) {
        rtc.SetIsWriteProtected(false);
      }
    
    if (!rtc.GetIsRunning()) {
        rtc.SetIsRunning(true);
      }
}


String getTime(){
    RtcDateTime time = rtc.GetDateTime();
    char buf[9];
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", time.Hour(), time.Minute(), time.Second());
    return String(buf);
}

String getDate(){
    RtcDateTime date = rtc.GetDateTime();
    char buf[11];
    snprintf(buf, sizeof(buf), "%02u/%02u/%04u", date.Day(), date.Month(), date.Year());
    return String(buf);

}

void setDateTime(int year, int month, int day, int hour, int min, int sec) {
    rtc.SetDateTime(RtcDateTime(year, month, day, hour, min, sec));
  }

