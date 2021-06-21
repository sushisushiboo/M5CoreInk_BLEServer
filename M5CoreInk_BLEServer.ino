#include <M5CoreInk.h>
#include <Preferences.h>
//#include <efontEnableAll.h>
#include <efontEnableAscii.h>
//#include <efontEnableJaMini.h>
#include <efontEnableJa.h>
#include <efont.h>
#include <efontM5StackCoreInk.h>
#include "ble.h"
#include "rtc.h"
#include "battery.h"
#include "ntp.h"

#define TIME_ACTIVE_MSEC 1 * 1000 // 動作時間
#define TIME_ACTIVE_NOTIFY_MSEC 60 * 1000 // 通知時の動作時間
#define TIME_SLEEP_SEC 30 // スリープ時間帯

Preferences preferences;
uint32_t tStart;
bool isDisplayInited = false;
bool notify = false;

Ink_Sprite inkSprite(&M5.M5Ink);

ble* myBle;
char message[BLE_MESSAEG_SIZE_MAX];

/**
 * ビープ
 */
void beep(uint32_t duration = 100) {
  M5.Speaker.tone(1200);
  delay(duration);
  M5.Speaker.mute();
}

/**
 * 画面初期化
 * @note 起動後描画前に実行する
 */
void initDisplay() {
  if (!isDisplayInited) {
    Serial.println("initDisplay");
    M5.M5Ink.clear();
    delay(1000);
  }
  isDisplayInited = true;
}

/**
 * 描画処理
 * @param [in] notify : 通知中
 */
void draw(bool notify = false) {
  char buffer[32];
  inkSprite.FillRect(0, 0, 200, 50, 1);
  // 日時
  getTimeString(buffer, sizeof(buffer));
  inkSprite.drawString(0, 0, buffer);
  if (notify) {
    inkSprite.drawString(180, 0, "!!");
  }
  preferences.putBool("notify", notify);
  // バッテリー
  bool batteryEnough =getBatteryStatus(buffer, sizeof(buffer), preferences.getBool("usbConnected"));
  preferences.putString("battery", buffer);
  inkSprite.drawString(0, 20, buffer);
  // メッセージ
  printEfont(&inkSprite, (char*)preferences.getString("message").c_str(), 0, 50, 2);
  // メッセージ日時
  inkSprite.drawString(0, 180, preferences.getString("time").c_str());

  if (!batteryEnough) {
    printEfont(&inkSprite, "  [充電して]  ", 0, 100, 2);
  }
  
  inkSprite.pushSprite();

  // バッテリーが空の時はシャットダウン
  if (!batteryEnough) {
    delay(2000);
    M5.shutdown();
  }
}

/**
 * 画面更新処理
 * @note バッテリー状態の更新のため1時間経過していたら更新する
 * @param [in] force : 強制更新
 * @param [in] notify: 通知中
 */
void updateDisplay(bool force = false, bool notify = false) {
  uint8_t hoursPast = preferences.getUChar("hours");
  RTC_TimeTypeDef rtcTime;
  M5.rtc.GetTime(&rtcTime);
  uint8_t hoursCurrent = rtcTime.Hours;
  // CHG表示の確認
  if ((0 < preferences.getString("battery").indexOf("CHG")) && !preferences.getBool("usbConnected")) {
    force = true; // 更新する
  }
  // NOTIFY表示の確認
  if (preferences.getBool("notify") != notify) {
    force = true; // 
  }
  if (force || (hoursPast != hoursCurrent) || notify) {
    preferences.putUChar("hours", hoursCurrent);  
    Serial.println("updateDisplay");
    initDisplay();
    draw(notify);
  }
}

/**
 * 電源断
 * @note USB給電時はリブートする
 */
void powerDown(uint16_t time = TIME_SLEEP_SEC) {
  Serial.println("powerDown");
  preferences.putBool("usbConnected", false); // USB給電状態を更新するために一旦クリア
  M5.shutdown(time);
  // 以下はUSB給電中のみ実行される
  beep();
  preferences.putBool("usbConnected", true);
  ESP.restart();
}

/**
 * NTPによる時刻更新
 * @note WiFi接続を利用する
 * @note 1ヶ月経過していら実行される
 */
void updateNtptime(bool force = false) {
  RTC_DateTypeDef rtcDate;
  M5.rtc.GetDate(&rtcDate);
  uint8_t monthPast = preferences.getUChar("month");
  if (force || (2021 > rtcDate.Year) || (monthPast != rtcDate.Month)) {
    tm timeinfo;
    if (getNtpTime(&timeinfo)) {
      setRtc(timeinfo);
      M5.rtc.GetDate(&rtcDate);
      preferences.putUChar("month", rtcDate.Month);
    }
  }  
}

void setup() {
  M5.begin(true, false, true);
  preferences.begin("bleserver2", false);
  if (!M5.M5Ink.isInit()) {
    Serial.println("Ink init failed");
  }
  if (inkSprite.creatSprite(0, 0, 200, 200, true) != 0) {
    Serial.println("Ink Sprite create failed");
  }  
  M5.Speaker.mute();

  updateNtptime();

  M5.update();
  if (M5.BtnEXT.isPressed()) {
    beep();
    // clear display
    preferences.putString("message", "");
    preferences.putString("time", "");
    updateDisplay(true);
  }
  if (M5.BtnPWR.isPressed()) {
    beep(500);
    // Change BLE Service to Notify
    notify = true;
  }
  myBle = new ble(&preferences, notify);

  updateDisplay(isEmpty(), notify);

  tStart = millis();
}

void loop() {
  if (M5.BtnPWR.wasPressed()) {
    Serial.println("BtnPWR pressed");
    beep();
    powerDown(1); // シャットダウン(1秒後に起動して状態更新）
  } else if (M5.BtnUP.wasPressed()) {
    Serial.println("BtnUP pressed");
    beep();
    updateNtptime(true); // 時刻合わせ
    updateDisplay(true);
  }
  if (!myBle->isConnected()) {
    uint32_t timeActive = TIME_ACTIVE_MSEC;
    bool usbConnected = preferences.getBool("usbConnected");
    if (notify && !usbConnected) {
      timeActive = TIME_ACTIVE_NOTIFY_MSEC;
    }
    if (timeActive <= (millis() - tStart)) {
      tStart = millis();
      if (usbConnected) {
        updateDisplay(true, notify);
      } else {
        if (!M5.BtnPWR.isPressed()) {
          powerDown();          
        }
      }
    }    
  } else {
    if (myBle->isReceiveMessage()) {
      myBle->getMessage(message, sizeof(message));
      preferences.putString("message", message);
      char buffer[32];
      getTimeString(buffer, sizeof(buffer));
      preferences.putString("time", buffer);
      updateDisplay(true, notify);
    }
    if (M5.BtnDOWN.wasPressed()) {
      beep();
      Serial.println("BtnDOWN pressed");
      myBle->notify("BtnDOWN pressed");
    }
  }
  M5.update();
}
