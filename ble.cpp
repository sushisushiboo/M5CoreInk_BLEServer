#include <M5CoreInk.h>
#include "ble.h"
#include "battery.h"

#define SERVICE_UUID        "1b62948e-7ccd-4b41-ad1b-181830a5742c"
#define SERVICE_UUID_NOTIFY "332451cb-ad46-4410-a1ae-b5e3f166ff1b"
#define CHARACTERISTIC_UUID "b6727492-b2ff-4563-aa2c-90c08e1bf879"

#define SERVICE_UUID_BATTERY "180f"
#define CHARACTERISTIC_UUID_BATTERYLEVEL "2a19"

class BLECharacteristicCallbacksBatteryLevel: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("#read battery level");
    float voltage, rate;
    getBattery(&voltage, &rate);
    int value = (int)rate;
    pCharacteristic->setValue(value);
  }
};

ble::ble(Preferences* pref, bool notify) {
  sprintf(message, "%s", pref->getString("message").c_str());
  
  Serial.print("BLE start.");
  if (notify) {
    Serial.println("(notify mode)");
  } else {
    Serial.println();
  }
  BLEDevice::init("M5StackCoreInk");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);
  BLEService *pService;
  if (notify) {
    pService = pServer->createService(SERVICE_UUID_NOTIFY);
  } else {
    pService = pServer->createService(SERVICE_UUID);
  }
  m_pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  m_pCharacteristic->setCallbacks(this);
  m_pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  // バッテリーサービス
  BLEService* pServiceBattery = pServer->createService(SERVICE_UUID_BATTERY); // battery service
  BLECharacteristic* pCharacteristicBattery = 
    pServiceBattery->createCharacteristic(
      CHARACTERISTIC_UUID_BATTERYLEVEL, // Battery Level
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_NOTIFY);    
  pCharacteristicBattery->setCallbacks(new BLECharacteristicCallbacksBatteryLevel());
  pCharacteristicBattery->addDescriptor(new BLE2902());
  pServiceBattery->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
}

ble::~ble() {
  
}

void ble::onConnect(BLEServer* pServer) {
  Serial.println("#connect");
  connected = true;
};

void ble::onDisconnect(BLEServer* pServer) {
  Serial.println("#disconnect");
  connected = false;
}

void ble::onRead(BLECharacteristic *pCharacteristic) {
  Serial.println("#read");
  pCharacteristic->setValue(message); 
}

void ble::onWrite(BLECharacteristic *pCharacteristic) {
  Serial.println("#write");
  std::string value = pCharacteristic->getValue();
  Serial.println(value.c_str());
  memset(message, 0, sizeof(message));
  if ((sizeof(message) - 1) < value.length()) {
    memcpy(message, value.c_str(), sizeof(message) - 1);
  } else {
    sprintf(message, "%s", value.c_str());
  }
  messageReceived = true;  
}

bool ble::isReceiveMessage() {
  return messageReceived;
}

void ble::getMessage(char* buffer, uint8_t length) {
  if (strlen(message) >= length) {
    return;
  }

  strcpy(buffer, message);
  messageReceived = false;
}

void ble::onIndicate(BLECharacteristic *pCharacteristic) {
  Serial.println("#indicate");
}

bool ble::isConnected() {
  return connected;
}

void ble::notify(const char* message) {
  if (connected) {
    m_pCharacteristic->setValue(message);
    m_pCharacteristic->notify();
  }
}
