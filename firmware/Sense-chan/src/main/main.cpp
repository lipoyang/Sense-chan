#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <Arduino.h>
#include "SenseChanFace.h"
#include <SoftwareSerial.h>
#include <SprReceiverBLE.h>
#include <Dynamixel2Arduino.h>

// ソフトウェアシリアルのピン
#define PIN_RX 2
#define PIN_TX 4
SoftwareSerial softSerial(PIN_RX, PIN_TX);

// 顔表示オブジェクト
SenseChanFace face;

// DYNAMIXEL設定
#define DXL_SERIAL   Serial2
#define DEBUG_SERIAL Serial
const int DXL_DIR_PIN = 5; // DYNAMIXEL Shield DIR PIN
const uint8_t DXL_ID_L = 1;
const uint8_t DXL_ID_R = 2;
const float DXL_PROTOCOL_VERSION = 2.0;
Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);
using namespace ControlTableItem; //This namespace is required to use Control table item names

// BLEラジコン受信器
SprReceiverBLE receiver;
static int g_fb = 0;  // 前後方向
static int g_lr = 0;  // 左右方向

// モーター制御関数
void ctrl_motor()
{
  int l = (int)(g_fb + g_lr/2);
  int r = (int)(g_fb - g_lr/2);

  // 仮
  l /= 2;
  r /= 2;
  dxl.setGoalVelocity(DXL_ID_L, +l, UNIT_PERCENT);
  dxl.setGoalVelocity(DXL_ID_R, -r, UNIT_PERCENT);
}


// 接続時
void onConnect()
{
  Serial.println("Connected!");
}

// 切断時
void onDisconnect()
{
  Serial.println("Disconnected!");
}

// 通信途絶時 (フェールセーフ処理)
void onLost()
{
  Serial.println("Lost!");
  g_fb = 0;
  g_lr = 0;
  ctrl_motor();
}

// スロットルコマンド受信時
// th : スロットル値 (-127～+127)
void onTH(int th)
{
  Serial.print("TH:");
  Serial.println(th);
  g_fb = th;
  ctrl_motor();
}

// ステアリングコマンド受信時
// st : ステアリング値 (-127～+127)
void onST(int st)
{
  Serial.print("ST:");
  Serial.println(st);
  g_lr = st;
  ctrl_motor();
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
  softSerial.begin(19200);
  receiver.onConnect = onConnect;
  receiver.onDisconnect = onDisconnect;
  receiver.onLost = onLost;
  receiver.onTH = onTH;
  receiver.onST = onST;
  receiver.begin(softSerial);

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
