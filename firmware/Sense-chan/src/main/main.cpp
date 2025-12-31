#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <Arduino.h>
#include <Dynamixel2Arduino.h>
#include "SenseChanFace.h"
#include "BleReceiver.h"

// 顔表示器
SenseChanFace face;

// BLEラジコン受信器
BleReceiver receiver;

// DYNAMIXEL設定
#define DXL_SERIAL   Serial2
const int DXL_DIR_PIN = 5; // DYNAMIXEL Shield DIR PIN
const uint8_t DXL_ID_L = 1;
const uint8_t DXL_ID_R = 2;
const float DXL_PROTOCOL_VERSION = 2.0;
Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);
using namespace ControlTableItem; //This namespace is required to use Control table item names

// BLEラジコン接続時
void onConnect()
{
  Serial.println("Connected!");
}

// BLEラジコン切断時
void onDisconnect()
{
  Serial.println("Disconnected!");
}

// BLEラジコン受信時
void onReceive(int l, int r)
{
  Serial.printf("Received: l=%d r=%d\n", l, r);
  // モータの速度制御
  // 対向二輪駆動なので極性に注意
  dxl.setGoalVelocity(DXL_ID_L, +l, UNIT_PERCENT);
  dxl.setGoalVelocity(DXL_ID_R, -r, UNIT_PERCENT);
}

// 初期化
void setup()
{
  Serial.begin(115200);
  
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  // BLEラジコン受信器の初期化
  receiver.onConnect = onConnect;
  receiver.onDisconnect = onDisconnect;
  receiver.onReceive = onReceive;
  receiver.begin();

  // 顔の初期化
  face.begin(); 

  // DYNAMIXELシリアルサーボの初期化
  dxl.begin(57600);
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);
  dxl.ping(DXL_ID_L);
  dxl.ping(DXL_ID_R);
  dxl.torqueOff(DXL_ID_L);
  dxl.torqueOff(DXL_ID_R);
  dxl.setOperatingMode(DXL_ID_L, OP_VELOCITY);
  dxl.setOperatingMode(DXL_ID_R, OP_VELOCITY);
  dxl.torqueOn(DXL_ID_L);
  dxl.torqueOn(DXL_ID_R);
}

// メインループ
void loop()
{
  // BLEラジコン受信器のメインループ処理
  receiver.loop();
}
