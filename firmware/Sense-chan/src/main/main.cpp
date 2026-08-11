#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <Arduino.h>
#include "SenseChanFace.h"
#include "Motor.h"
#include "BleReceiver.h"
#include "BatteryCheck.h"
#include "SdCard.h"
#include "SerialCmd.h"
#include "VoiceDetector.h"

// スタックチャンの顔表示器
SenseChanFace face;
// モータ制御
Motor motor;
// BLEラジコン受信器
BleReceiver receiver;
// バッテリー電圧監視
BatteryCheck batteryCheck;
// 音声コマンド検出器
VoiceDetector vd;
// SDカード
SdCard sdCard;
// USBシリアルコマンド (開発用)
SerialCmd serialCmd;

// BLEラジコン接続時
void onConnect()
{
  Serial.println("Connected!");
  face.setMicroMotion(false);
  face.setBaseExpression(Expression::Neutral);
  face.setExpression(Expression::Happy, 2000);
  face.setSpeachText("プロポ接続したよ", 2000);

  // DYNAMIXELシリアルサーボを速度制御に変更
  motor.setVelocityMode();
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
  motor.setPositionMode();
}

// BLEラジコン受信時
void onReceive(int l, int r)
{
  Serial.printf("Received: l=%d r=%d\n", l, r);

  // モータの速度制御
  motor.setVelocity((float)l, (float)r);
}

// バッテリー電圧監視コールバック
void onBatteryCheck(float voltage, bool wasLowBattery)
{
  //Serial.printf("Battery Voltage: %.2f V\n", voltage);

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
  // Serial.printf("MicroMotion: x=%.2f y=%.2f\n", x, y);
  
  // モータの位置制御  
  motor.setPosition(x, y);
}

// シリアルコマンド (開発用)
void onCommand(int argc, char const *argv[])
{
  if(argc <= 0) return;

  // パラメータ設定
  if(strcmp(argv[0], "set") == 0 && argc == 3)
  {
    if(strcmp(argv[1], "Kx") == 0) {
      motor.parameter.Kx = atof(argv[2]);
      motor.setParameter();
      Serial.printf("Set Kx = %.2f\n", motor.parameter.Kx);
    }
    else if(strcmp(argv[1], "Ky") == 0) {
      motor.parameter.Ky = atof(argv[2]);
      motor.setParameter();
      Serial.printf("Set Ky = %.2f\n", motor.parameter.Ky);
    }
    else if(strcmp(argv[1], "Vmax") == 0) {
      motor.parameter.Vmax = atof(argv[2]);
      motor.setParameter();
      Serial.printf("Set Vmax = %.2f\n", motor.parameter.Vmax);
    }
    else if(strcmp(argv[1], "Vfull") == 0) {
      motor.parameter.Vfull = atof(argv[2]);
      motor.setParameter();
      Serial.printf("Set Vfull = %.2f\n", motor.parameter.Vfull);
    }
  }
  // パラメータ表示
  else if(strcmp(argv[0], "print") == 0 && argc == 1)
  {
    Serial.printf("Kx=%.2f Ky=%.2f Vmax=%.2f Vfull=%.2f\n",
    motor.parameter.Kx, motor.parameter.Ky, motor.parameter.Vmax, motor.parameter.Vfull);
  }
  // パラメータ保存
  else if(strcmp(argv[0], "save") == 0 && argc == 1)
  {
    sdCard.Kx = motor.parameter.Kx;
    sdCard.Ky = motor.parameter.Ky;
    sdCard.Vmax = motor.parameter.Vmax;
    sdCard.Vfull = motor.parameter.Vfull;
    if(sdCard.save()) {
      Serial.println("Settings saved to SD card.");
    } else {
      Serial.println("Failed to save settings to SD card.");
    }
  }
  // おすわり
  else if(strcmp(argv[0], "sit") == 0 && argc == 1)
  {
    motor.setPause(true);
    Serial.println("Sit down!");
  }
  // おすわり解除
  else if(strcmp(argv[0], "ok") == 0 && argc == 1)
  {
    motor.setPause(false);
    Serial.println("OK!");
  }
  // 音声コマンド登録
  else if(strcmp(argv[0], "regist") == 0 && argc == 2)
  {
    int command_no = atoi(argv[1]);
    if(command_no >=0 && command_no <= 4){
      Serial.printf("Voice command [%d] registing...\n", command_no);
      vd.regist(command_no);
    }else{
      Serial.printf("Bad voice command number (%d)\n", command_no);
    }
  }
  // 音声コマンド検出
  else if(strcmp(argv[0], "detect") == 0 && argc == 1)
  {
    Serial.println("Voice command detecting...");
      vd.detect();
  }
  // 音声コマンド登録/検出のキャンセル
  else if(strcmp(argv[0], "cancel") == 0 && argc == 1)
  {
    Serial.println("Voice command canceled!");
      vd.cancel();
  }
  else{
    Serial.println("unknown command");
  }
}

// 音声コマンド登録通知
void onRegist(uint32_t commnad_no)
{
  if(commnad_no < MAX_COMMAND){
    printf("Voice Command Registed! (%ld)\n", commnad_no);
  }else{
    printf("Voice Command Regist ERROR! (%ld)\n", commnad_no);
  }
}

// 音声コマンド検出通知
void onDetect(uint32_t commnad_no)
{
  if(commnad_no < MAX_COMMAND){
    printf("Voice Command Detected! (%ld)\n", commnad_no);
  }else if(commnad_no == MFCC_MISMATCH){
    printf("Voice Command mismatch\n");
  }else{
    printf("Voice Command Detect ERROR! (%ld)\n", commnad_no);
  }
}

// 初期化
void setup()
{
  serialCmd.onCommand = onCommand;
  serialCmd.begin();

  // SDカードの初期化と設定読み込み
  if(sdCard.begin()){
    sdCard.load();
    motor.parameter.Kx   = sdCard.Kx;
    motor.parameter.Ky   = sdCard.Ky;
    motor.parameter.Vmax = sdCard.Vmax;
    motor.parameter.Vfull = sdCard.Vfull;
    face.pwmBL = sdCard.PWM;
    receiver.RX = sdCard.RX;
    receiver.TX = sdCard.TX;
    receiver.Baud = sdCard.Baud;
  }
  
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
  motor.begin();

  // バッテリー電圧監視の初期化
  batteryCheck.begin();
  batteryCheck.onBatteryCheck = onBatteryCheck;

  // 音声コマンド検出器の初期化
  //vd.onRegist = onRegist;
  //vd.onDetect = onDetect;
  //vd.begin();
}

// メインループ
void loop()
{
  // BLEラジコン受信器
  receiver.loop();
  // スタックチャンの顔
  face.loop();
  // バッテリー電圧監視
  batteryCheck.loop();
  // モータ制御
  motor.loop();
  // シリアルコマンド受信
  serialCmd.loop();
  // 音声コマンド検出器
  //vd.loop();

  usleep(1000);
}
