/**
 * rtc.cpp
 * RTCへの時刻の取得設定
 */

#include <M5CoreInk.h>
#include "rtc.h"

RTC_DateTypeDef rtcDate;
RTC_TimeTypeDef rtcTime;

/**
 * 時刻文字列の取得
 */
void getTimeString(char* buffer, uint8_t length) {
  M5.rtc.GetTime(&rtcTime);
  M5.rtc.GetDate(&rtcDate);

  if (20 > length) {
    return;
  }
  sprintf(buffer, "%d/%02d/%02d %02d:%02d:%02d",
    rtcDate.Year, rtcDate.Month, rtcDate.Date,
    rtcTime.Hours, rtcTime.Minutes, rtcTime.Seconds);
}

/**
 * RTCへの時刻の設定
 */
void setRtc(tm timeinfo) {
  rtcTime.Minutes = timeinfo.tm_min;
  rtcTime.Seconds = timeinfo.tm_sec;
  rtcTime.Hours = timeinfo.tm_hour;
  rtcDate.Year = timeinfo.tm_year + 1900;
  rtcDate.Month = timeinfo.tm_mon + 1;
  rtcDate.Date = timeinfo.tm_mday;
  rtcDate.WeekDay = timeinfo.tm_wday;
  M5.rtc.SetTime(&rtcTime);
  M5.rtc.SetDate(&rtcDate);
}
