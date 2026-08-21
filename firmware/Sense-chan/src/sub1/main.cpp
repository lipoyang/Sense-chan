#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

#include <Arduino.h>
#include <MP.h>
#include <Dynamixel2Arduino.h>
#include "IMU.h"
#include "PollingTimer.h"

// メインコアID
const int MAINCORE_ID = 0;

// メッセージID定義
const int8_t MSGID_BEGUN = 1;
const int8_t MSGID_SET_PARAMETER = 2;
const int8_t MSGID_SET_VELOCITY_MODE = 3;
const int8_t MSGID_SET_POSITION_MODE = 4;
const int8_t MSGID_SET_VELOCITY = 5;
const int8_t MSGID_SET_POSITION = 6;
const int8_t MSGID_SET_PAUSE = 7;
const int8_t MSGID_SET_MODE = 8;

// モード
#define MODE_UROCHORO   0 // うろちょろ
#define MODE_RADIDON    1 // ラジコン
#define MODE_GYRO       2 // ジャイロ

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
float Kx = 120.0f;  // 旋回成分の係数 [度]
float Ky = 60.0f;   // 並進成分の係数 [度]
float Vmax = 2.0f;  // 位置制御の台形制御の最大速度
float Vfull = 12.0f; // 速度制御の最大速度 [%]
float Kp = 0.25f;  // PID制御の比例ゲイン
float Ki = 0.0f;   // PID制御の積分ゲイン
float Kd = 0.25f;  // PID制御の微分ゲイン
bool pausing = false; // 一時停止中か？
float ie = 0.0f; // PID制御の積分値
int mode = MODE_UROCHORO; // モード

// IMUセンサ関連
IMU imu;

// エラーループ
void errorLoop(int num)
{
  int i;
  while (1) {
    for (i = 0; i < num; i++) {
      digitalWrite(LED1, HIGH);
      delay(300);
      digitalWrite(LED1, LOW);
      delay(300);
    }
    delay(1000);
  }
}

// モータ制御
void servoControl()
{
  if(pausing) return; // 一時停止中

  // 位置制御モードか？
  if(servoMode == OP_POSITION)
  {
    for(uint8_t i = 0; i < 2; i++) {
      uint8_t id = DXL_ID[i];

      float diff = posTarget[i] - posCurrent[i];
      if (diff >  Vmax) diff =  Vmax;
      if (diff < -Vmax) diff = -Vmax;
      posCurrent[i] += diff;
      dxl.setGoalPosition(id, posOffset[i] + posCurrent[i], UNIT_DEGREE);
    }
  }
  // 速度制御モードか？
  else if(servoMode == OP_VELOCITY)
  {
    // ジャイロモードか？
    if(mode == MODE_GYRO){
      // 方位角の取得
      float theta = imu.getTheta();
      float gyro = imu.getGyro();
      static int cnt = 0;

      // 速度制御モード
      if(theta > 5.0f || theta < -5.0f) {
        ie = 0.0f; // 積分値をリセット (Anti-windup)
      }else{
        ie += theta;
      }

      float v =  -Kp * theta - Ki * ie - Kd * gyro;
      dxl.setGoalVelocity(DXL_ID[0], v, UNIT_PERCENT);
      dxl.setGoalVelocity(DXL_ID[1], v, UNIT_PERCENT);

      if(++cnt >= 50) {
        cnt = 0;
        // MPLog("theta=%.2f, v=%.2f, ie=%.2f, gyro=%.2f\n", theta, v, ie, gyro);
      }
    }
  }
}

// 初期化
void setup()
{
  // サブコア開始
  int ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }

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

  // IMUセンサの初期化
  if(!imu.begin()){
    MPLog("IMU: begin error");
    // errorLoop(3);
  }
  imu.calibrate(); // キャリブレーション開始
  
  // 初期化完了をメインコアに知らせる
  uint32_t dummy = 0;
  MP.Send(MSGID_BEGUN, dummy, MAINCORE_ID);

  // パラメータ設定
  MP.RecvTimeout(MP_RECV_BLOCKING);
  int8_t msgid;
  struct Parameter{
    float Kx;   // 旋回成分の係数 [度]
    float Ky;   // 並進成分の係数 [度]
    float Vmax; // // 位置制御の台形制御の最大速度
    float Vfull; // 速度制御の最大速度 [%]
    float Kp;   // PID制御の比例ゲイン
    float Ki;   // PID制御の積分ゲイン
    float Kd;   // PID制御の微分ゲイン
  };
  Parameter* pParam;
  MP.Recv(&msgid, &pParam);
  if (msgid != MSGID_SET_PARAMETER) {
      MPLog("Motor: MP.Recv error: no SET_PARAMETER message %d\n", msgid);
  }
  Kx = pParam->Kx;
  Ky = pParam->Ky;
  Vmax = pParam->Vmax;
  Vfull = pParam->Vfull;
  Kp = pParam->Kp;
  Ki = pParam->Ki;
  Kd = pParam->Kd;
  MP.Send(MSGID_SET_PARAMETER, dummy, MAINCORE_ID);

  MP.RecvTimeout(MP_RECV_POLLING);

  // モータ制御用タイマ
  servoTimer.set(20);
}

// メインループ
void loop()
{
  // メッセージID
  int8_t msgid;

  // メッセージデータ
  typedef struct {
      float l;
      float r;
  } S_Velocity;

  typedef struct {
      float x;
      float y;
  } S_Position;

  typedef struct{
    float Kx;
    float Ky;
    float Vmax;
    float Vfull;
    float Kp;
    float Ki;
    float Kd;
  } S_Parameter;

  typedef struct{
    bool pause;
  } S_Pause;

  typedef struct {
    int mode;
  } S_SetMode;

  typedef union {
    S_Velocity velocity;
    S_Position position;
    S_Parameter parameter;
    S_Pause pause;
    S_SetMode mode;
  } MsgData;
  MsgData *msgdata;

  // メッセージ受信
  int ret = MP.Recv(&msgid, &msgdata);
  switch(ret){
    case MSGID_SET_PARAMETER:
      // パラメータ設定
      Kx   = msgdata->parameter.Kx;
      Ky   = msgdata->parameter.Ky;
      Vmax = msgdata->parameter.Vmax;
      Vfull = msgdata->parameter.Vfull;
      Kp = msgdata->parameter.Kp;
      Ki = msgdata->parameter.Ki;
      Kd = msgdata->parameter.Kd;
      break;
    case MSGID_SET_VELOCITY_MODE:
      // DYNAMIXELシリアルサーボを速度制御に変更
      for(uint8_t i = 0; i < 2; i++) {
        uint8_t id = DXL_ID[i];
        dxl.torqueOff(id);
        dxl.setGoalVelocity(id, 0, UNIT_PERCENT);
        dxl.setOperatingMode(id, OP_VELOCITY);
        dxl.torqueOn(id);
      }
      servoMode = OP_VELOCITY;
      break;
    case MSGID_SET_POSITION_MODE:
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
      break;
    case MSGID_SET_VELOCITY:
      {
        // モータの速度制御
        float l = Vfull * msgdata->velocity.l / 127.0f;
        float r = Vfull * msgdata->velocity.r / 127.0f;
        // 対向二輪駆動なので極性に注意
        dxl.setGoalVelocity(DXL_ID[0], +l, UNIT_PERCENT);
        dxl.setGoalVelocity(DXL_ID[1], -r, UNIT_PERCENT);
      }
      break;
    case MSGID_SET_POSITION:
      {
        // モータの位置制御
        float x = msgdata->position.x;
        float y = msgdata->position.y;
        posTarget[0] =  Kx * x + Ky * y; // 左
        posTarget[1] = -Kx * x + Ky * y; // 右
      }
      break;
    case MSGID_SET_PAUSE:
      {
        // 一時停止/解除
        pausing = msgdata->pause.pause;
        if(pausing == false){
          for(uint8_t i = 0; i < 2; i++) {
            uint8_t id = DXL_ID[i];
            posOffset[i] = dxl.getPresentPosition(id, UNIT_DEGREE);
            posTarget[i] = 0.0f;
            posCurrent[i] = 0.0f;
            dxl.setGoalPosition(id, posOffset[i], UNIT_DEGREE);
          }
        }
      }
      break;
    case MSGID_SET_MODE:
      {
        int val = msgdata->mode.mode;
        if(val >= MODE_UROCHORO && val <= MODE_GYRO){
          if(val != mode){
            mode = val;
            MPLog("mode = %d\n", mode);

            if(mode == MODE_UROCHORO){
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
            }else{
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
          }
        }
      }
      break;
  }

  // モータ制御
  if(servoTimer.elapsed()) {
    servoControl();
    imu.update();
  }
}
