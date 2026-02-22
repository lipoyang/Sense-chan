#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <Arduino.h>
#include <Dynamixel2Arduino.h>
#include "SenseChanFace.h"
#include "BleReceiver.h"
#include "BatteryCheck.h"

// スタックチャンの顔表示器
SenseChanFace face;

// BLEラジコン受信器
BleReceiver receiver;

// バッテリー電圧監視
BatteryCheck batteryCheck;

// DYNAMIXEL設定
#define DXL_SERIAL   Serial2  // シリアルポート
const int DXL_DIR_PIN = 5;    // 半二重通信の方向制御ピン
const uint8_t DXL_ID_L = 1;   // 左モータID
const uint8_t DXL_ID_R = 2;   // 右モータID
const float DXL_PROTOCOL_VERSION = 2.0; // プロトコルバージョン
Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);
using namespace ControlTableItem;

// BLEラジコン接続時
void onConnect()
{
  Serial.println("Connected!");
  face.setMicroMotion(false);
  face.setBaseExpression(Expression::Neutral);
  face.setExpression(Expression::Happy, 2000);
  face.setSpeachText("プロポ接続したよ", 2000);
}

// BLEラジコン切断時
void onDisconnect()
{
  Serial.println("Disconnected!");
  face.setBaseExpression(Expression::Neutral); // Sleepy);
  face.setExpression(Expression::Neutral, 2000);
  face.setSpeachText("プロポ切断したよ", 2000);
  face.setMicroMotion(true);
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

// バッテリー電圧監視コールバック
void onBatteryCheck(float voltage, bool wasLowBattery)
{
  Serial.printf("Battery Voltage: %.2f V\n", voltage);

  if(voltage < LOW_BATTERY) {
    face.setExpression(Expression::Sad, 2000);
    face.setSpeachText("電圧が下がってるよ", 2000);
  }else if(wasLowBattery) {
    face.setExpression(Expression::Neutral, 2000);
    face.clearSpeachText();
  }
}

// スタックチャンの微動コールバック
void onMicroMotion(float x, float y)
{
    Serial.printf("MicroMotion: x=%.2f y=%.2f\n", x, y);
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

  // スタックチャンの顔の初期化
  face.onMicroMotion = onMicroMotion;
  face.begin();
  face.setBaseExpression(Expression::Neutral); // Sleepy);
  face.setExpression(Expression::Happy, 2000);
  face.setMicroMotion(true);

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

  // バッテリー電圧監視の初期化
  batteryCheck.begin();
  batteryCheck.onBatteryCheck = onBatteryCheck;
}

// メインループ
void loop()
{
  // BLEラジコン受信器のメインループ処理
  receiver.loop();

  // スタックチャンの顔のメインループ処理
  face.loop();

  // バッテリー電圧監視のメインループ処理
  batteryCheck.loop();
}
