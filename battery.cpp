/**
 * battery.cpp
 * バッテリー電圧を取得する
 */

#include "M5CoreInk.h"
#include "esp_adc_cal.h"
#include "battery.h"

#define BATTERY_VOLTAGE_MAX 4.2 // カタログ値は3.7V
#define BATTERY_VOLTAGE_MIN 3.2

/**
 * バッテリー状態の取得
 * @param [out] voltage: 電圧
 * @param [out] rate: 割合
 * @return : バッテリー状態(true: OK, false: 空）
 */
bool getBattery(float* voltage, float* rate) {
    analogSetPinAttenuation(35,ADC_11db);
    esp_adc_cal_characteristics_t *adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 3600, adc_chars);
    uint16_t ADCValue = analogRead(35);
    
    uint32_t BatVolmV  = esp_adc_cal_raw_to_voltage(ADCValue,adc_chars);
    *voltage = float(BatVolmV) * 25.1 / 5.1 / 1000;
    *rate = (*voltage - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN) * 100;

    if (*rate <= 0.0) {
      return false;
    }

    return true;
}

/**
 * バッテリー状態の取得（文字列）
 * @param [out] buf : 文字列
 * @param [in] length : bufのサイズ
 * @param [in] charge : 充電中
 * @return : バッテリー状態(true: OK, false: 空）
 */
bool getBatteryStatus(char* buf, uint8_t length, bool charge) {
  float voltage;
  float rate;
  char chg[8] = {0};
  if (charge) {
    sprintf(chg, "[CHG]");
  }
  bool status = getBattery(&voltage, &rate);
  if (status) {
    sprintf(buf, "%.2fV %.1f%% %s", voltage, rate, chg);  
  } else {
    sprintf(buf, "%.2fV %.1f%% EMP %s", voltage, rate, chg);
  }
  return status;
}
