#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define BLE_MESSAEG_SIZE_MAX 65

class ble: public BLECharacteristicCallbacks, public BLEServerCallbacks {
public:
  ble(bool notify = false);
  ~ble();

  void onConnect(BLEServer* pServer);
  void onDisconnect(BLEServer* pServer);
  void onRead(BLECharacteristic *pCharacteristic);
  void onWrite(BLECharacteristic *pCharacteristic);
  void onIndicate(BLECharacteristic *pCharacteristic);

  bool isConnected();
  bool isReceiveMessage();
  void getMessage(char* buffer, uint8_t length);
  void notify(const char* message);

private:
  BLEServer* pServer = NULL;
  BLECharacteristic* pCharacteristic = NULL;
  char message[BLE_MESSAEG_SIZE_MAX];
  bool messageReceived = false;
  bool connected = false;
};
