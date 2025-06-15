#include <virtuabotixRTC.h>

#define clk 26
#define dat 27
#define rst 25

virtuabotixRTC myRTC(clk, dat, rst);

void setup() {
  Serial.begin(115200);
}

void loop() {
  DS1302_GetDateAndTime();
  delay(2000);
}

void DS1302_GetDateAndTime() {
  myRTC.updateTime();
  char timeStamp[20] = {};
  sprintf(timeStamp, "%04d-%02d-%02dT%02d:%02d:%02d", myRTC.year, myRTC.month, myRTC.dayofmonth, myRTC.hours, myRTC.minutes, myRTC.seconds);
  Serial.println(timeStamp);
}
