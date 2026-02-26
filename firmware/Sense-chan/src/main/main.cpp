#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <Arduino.h>
#include <Dynamixel2Arduino.h>
#include "SenseChanFace.h"
#include "BleReceiver.h"
#include "BatteryCheck.h"
#include "PollingTimer.h"

// スタックチャンの顔表示器
SenseChanFace face;

// BLEラジコン受信器
BleReceiver receiver;

// バッテリー電圧監視
BatteryCheck batteryCheck;

// DYNAMIXEL設定
#define DXL_SERIAL   Serial2  // シリアルポート
const int DXL_DIR_PIN = 5;    // 半二重通信の方向制御ピン
const uint8_t DXL_ID[2] = {1, 2} ;   // 左右のモータID
const float DXL_PROTOCOL_VERSION = 2.0; // プロトコルバージョン
Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);
using namespace ControlTableItem;

// モータ制御関連
IntervalTimer servoTimer;     // 周期タイマ
int servoMode = OP_POSITION;  // 制御モード
float posOffset[2] = {0.0f, 0.0f}; // 位置オフセット
float posTarget[2] = {0.0f, 0.0f}; // 目標値
float posCurrent[2] = {0.0f, 0.0f}; // 現在値

// BLEラジコン接続時
void onConnect()
{
  Serial.println("Connected!");
  face.setMicroMotion(false);
  face.setBaseExpression(Expression::Neutral);
  face.setExpression(Expression::Happy, 2000);
  face.setSpeachText("プロポ接続したよ", 2000);

  // DYNAMIXELシリアルサーボを速度制御に変更
  for(uint8_t i = 0; i < 2; i++) {
    uint8_t id = DXL_ID[i];
    dxl.torqueOff(id);
    dxl.setGoalVelocity(id, 0, UNIT_PERCENT);
    dxl.setOperatingMode(id, OP_VELOCITY);
    dxl.torqueOn(id);
  }
  servoMode = OP_VELOCITY;
}

// BLEラジコン切断時
void onDisconnect()
{
  Serial.println("Disconnected!");
  face.setBaseExpression(Expression::Neutral); // Sleepy);
  face.setExpression(Expression::Neutral, 2000);
  face.setSpeachText("プロポ切断したよ", 2000);
  face.setMicroMotion(true);

  // DYNAMIXELシリアルサーボを位置制御に変更
  for(uint8_t i = 0; i < 2; i++) {
    uint8_t id = DXL_ID[i];
    dxl.torqueOff(id);
    dxl.setOperatingMode(id, OP_POSITION);
    posOffset[i] = dxl.getPresentPosition(id, UNIT_DEGREE);
    posTarget[i] = 0.0f;
    posCurrent[i] = 0.0f;
    dxl.setGoalPosition(id, posOffset[i], UNIT_DEGREE);
    dxl.torqueOn(id);
  }
  servoMode = OP_POSITION;
}

// BLEラジコン受信時
void onReceive(int l, int r)
{
  Serial.printf("Received: l=%d r=%d\n", l, r);
  // モータの速度制御
  // 対向二輪駆動なので極性に注意
  dxl.setGoalVelocity(DXL_ID[0], +l, UNIT_PERCENT);
  dxl.setGoalVelocity(DXL_ID[1], -r, UNIT_PERCENT);
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

  // 目標位置の設定
  const float Kx = 120.0f;  // 旋回成分の係数 [度]
  const float Ky = 90.0f;   // 並進成分の係数 [度]

  posTarget[0] =  Kx * x + Ky * y; // 左
  posTarget[1] = -Kx * x + Ky * y; // 右
}

// モータ制御
void servoControl()
{
  const float Amax = 10.0f;

  // 位置制御モードか？
  if(servoMode == OP_POSITION)
  {
    for(uint8_t i = 0; i < 2; i++) {
      uint8_t id = DXL_ID[i];

      float diff = posTarget[i] - posCurrent[i];
      if (diff >  Amax) diff =  Amax;
      if (diff < -Amax) diff = -Amax;
      posCurrent[i] += diff;
      dxl.setGoalPosition(id, posOffset[i] + posCurrent[i], UNIT_DEGREE);
    }
  }
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

  // DYNAMIXELシリアルサーボの初期化 (位置制御)
  dxl.begin(57600);
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);
  for(uint8_t i = 0; i < 2; i++) {
    uint8_t id = DXL_ID[i];
    dxl.ping(id);
    dxl.torqueOff(id);
    dxl.setOperatingMode(id, OP_POSITION);
    posOffset[i] = dxl.getPresentPosition(id, UNIT_DEGREE);
    posTarget[i] = 0.0f;
    posCurrent[i] = 0.0f;
    dxl.setGoalPosition(id, posOffset[i], UNIT_DEGREE);
    dxl.torqueOn(id);
  }
  servoMode = OP_POSITION;

  // バッテリー電圧監視の初期化
  batteryCheck.begin();
  batteryCheck.onBatteryCheck = onBatteryCheck;

  // モータ制御用タイマ
  servoTimer.set(20);
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

  // モータ制御
  if(servoTimer.elapsed()) {
    servoControl();
  }
}
