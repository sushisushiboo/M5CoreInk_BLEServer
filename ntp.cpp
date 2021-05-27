/**
 * ntp.cpp
 * NTP時刻合わせを行う
 */

#include <M5CoreInk.h>
#include <WiFi.h>

#define WIFI_SSID "airmac-toshi1-g"
#define WIFI_PASSWORD "kotona20021226"

#define JST     3600*9

bool wifiConnect() {
  Serial.print("connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t tStart = millis();
  do {
    switch(WiFi.status()) {
      case WL_CONNECTED:
        Serial.println("connected");
        Serial.println(WiFi.localIP());
        Serial.println(WiFi.macAddress());
        return true;
      case WL_CONNECT_FAILED:
        Serial.println("failed");
        return false;
      default:
        break;
    }

    Serial.print(".");
    delay(500);
  } while(10000 > (millis() - tStart));

  Serial.println("timeout");
  return false;
}

bool ntp(tm* timeinfo) {
  // config NTP
  configTime(JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  // get time from NTP
  Serial.print("ntp sync");
  uint32_t tStart = millis();
  do {
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
    if (2021 <= (timeinfo->tm_year + 1900)) {
      Serial.println("OK");
      Serial.println(now);
      return true;
    }
    Serial.print(".");
    delay(10);
  } while (10000 > (millis() - tStart));

  Serial.println("timeout");
  return false;
}

/**
 * NTPから時刻を取得
 * @note WiFi接続してやる
 * @param [out] timeinfo
 */
bool getNtpTime(tm* timeinfo) {
  Serial.println("NTP by WiFi start");
  // connect Wifi
  if (false == wifiConnect()) {
    return false;
  }
  // ntp
  if (false == ntp(timeinfo)) {
    return false;
  }
  WiFi.disconnect();
  return true;
}
