#include <MP.h>
#include <Wire.h>
#include <BMI160Gen.h>
#include "IMU.h"

#define GYRO_RANGE 2000.0f // ジャイロのレンジ [deg/s]

// 初期化する
bool IMU::begin()
{
  // 変数の初期化
  theta = 0.0;
  calibCnt = -1;

  // BMI160の初期化
  BMI160.begin(BMI160GenClass::I2C_MODE, 0x69);

  // デバイスIDを取得
  const uint8_t DEV_ID = 0xD1;
  uint8_t dev_id = BMI160.getDeviceID();
  MPLog("DEVICE ID: %02X\n", dev_id);
  if(dev_id != DEV_ID){
    MPLog("Gyro Error!\n");
    _isError = true;
    return false;
  }

  // ジャイロのレンジを設定
  BMI160.setGyroRange((int)GYRO_RANGE);
  MPLog("Initializing IMU device...done.\n");

  _isError = false;
  return true;
}

// 更新する
void IMU::update()
{
  // ジャイロの値を取得し、deg/sec に変換する
  int gxRaw, gyRaw, gzRaw;
  BMI160.readGyro(gxRaw, gyRaw, gzRaw);
  float g = gzRaw * GYRO_RANGE / 32768.0f;

  // キャリブレーション中でないなら、角速度を積分して方位角を更新する
  if(calibCnt < 0){
    g -= g0; // オフセットを引く
    theta += g * 0.020f; // 累積して方位角を更新
    gyro = g;
  }
  // キャリブレーション中
  else if(calibCnt < 100){
    g0 += g;
    calibCnt++;
    theta = 0.0f;
    gyro = 0.0f;
  }
  // キャリブレーション完了
  if(calibCnt == 100){
    g0 /= 100.0f;
    MPLog("Gyro offset: %.7f\n", g0);
    calibCnt = -1;
  }
}
