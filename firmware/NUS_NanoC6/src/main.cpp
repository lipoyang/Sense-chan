#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h> // Notifyに必要

// デバッグ用マクロ
#define DEBUG_LOG
#if defined(DEBUG_LOG)
  #define LOG_BEGIN(x)      Serial.begin(x)
  #define LOG_PRINTLN(x)    Serial.println(x)
  #define LOG_PRINT(x)      Serial.print(x)
#else
  #define LOG_BEGIN(x)
  #define LOG_PRINTLN(x)
  #define LOG_PRINT(x)
#endif

// シリアルポートのピン定義 (Groveコネクタ)
#define PIN_RX 2
#define PIN_TX 1

// Nordic UART Service (NUS) のUUID
#define SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHAR_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // M5からみたRX
#define CHAR_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // M5がらみたTX

// キャラクタリスティック
BLECharacteristic *pCharRX; // M5からみたRX
BLECharacteristic *pCharTX; // M5がらみたTX

// 接続状態管理用フラグ
bool isConnected = false;
bool wasConnected = false;

// 接続時/切断時のコールバック
class MyServerCallbacks: public BLEServerCallbacks {
    // 接続時
    void onConnect(BLEServer* pServer) {
      isConnected = true;
      LOG_PRINTLN("Connected");
    }
    // 切断時
    void onDisconnect(BLEServer* pServer) {
      isConnected = false;
      LOG_PRINTLN("Disconnected");
    }
};

// データ受信時のコールバック
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharRX) {
      String rxValue = pCharRX->getValue();

      if (rxValue.length() > 0) {
        LOG_PRINT("Received: ");
        LOG_PRINTLN(rxValue);
        Serial1.print(rxValue); 
      }
    }
};

// 初期化
void setup()
{
  // デバッグ用USBシリアル
  LOG_BEGIN(115200);

  // ハードウェアシリアルポート (Groveコネクタ)
  Serial1.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);

  // BLEデバイスを初期化
  BLEDevice::init("M5 NUS");

  // GATTサーバー
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // NUSサービス
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // TXキャラクタリスティック (Notify : M5 -> Central)
  pCharTX = 
      pService->createCharacteristic(
        CHAR_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
      );
  pCharTX->addDescriptor(new BLE2902()); // Notifyを有効にするためのデスクリプタ追加

  // RXキャラクタリスティック (Write : Central -> M5)
  pCharRX = 
      pService->createCharacteristic(
        CHAR_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
      );
  pCharRX->setCallbacks(new MyCallbacks());

  // サービスを開始
  pService->start();

  // アドバタイジングの設定
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0); // iOSとの接続のために推奨
  
  // アドバタイジング周期の設定 (100ms)
  uint16_t advInterval = 160; // 100ms / 0.625ms = 160
  pAdvertising->setMinInterval(advInterval);
  pAdvertising->setMaxInterval(advInterval);

  // アドバタイジングを開始
  BLEDevice::startAdvertising();
}

// メインループ
void loop()
{
  // 接続状態の変化をチェック
  if (isConnected && !wasConnected) {
    wasConnected = isConnected;
  }
  if (!isConnected && wasConnected) {
      wasConnected = isConnected;
      // 切断されたらアドバタイジングを再開
      BLEDevice::startAdvertising(); 
      LOG_PRINTLN("Restart Advertising");
  }

  // シリアル受信データをBLE送信
  if (isConnected) {
    int len = Serial1.available();
    if (len > 0) {
      if(len > 255) len = 255;
      static char rxBuffer[256];
      int len2 = Serial1.readBytes(rxBuffer, len);
      rxBuffer[len2] = '\0';
      String txString = String(rxBuffer);
      // String txString = Serial.readString(); // <-- こっちだと遅延が大きい
      pCharTX->setValue(txString);
      pCharTX->notify();
    }
  }
  delay(5);
}
