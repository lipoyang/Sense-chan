#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <MP.h>
#include "IMU.h"

// BNO055 デバイス (ID, I2Cアドレス, I2Cバス)
Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);

// 初期化する
bool IMU::begin()
{
  // BNO055を9軸フルフュージョンモードで開始
  if(!bno.begin(OPERATION_MODE_NDOF))
  {
    MPLog("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    return false;
  }
  MPLog("BNO055 detected!");
  // Wire.setClock(400000);

  bno.setExtCrystalUse(true);

  return true;
}

// キャリブレーション済みかどうかを返す
bool IMU::isCalibrated()
{
  uint8_t system, gyro, accel, mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag);
//return (system == 3 && gyro == 3 && accel == 3 && mag == 3);
  return (gyro == 3);
}

// 温度を取得する
int IMU::getTemperature()
{
    int8_t temp = bno.getTemp();
    return temp;
}

// 方位角を取得する
float IMU::getHeading()
{
  // Possible vector values can be:
  // - VECTOR_ACCELEROMETER - m/s^2
  // - VECTOR_MAGNETOMETER  - uT
  // - VECTOR_GYROSCOPE     - rad/s
  // - VECTOR_EULER         - degrees
  // - VECTOR_LINEARACCEL   - m/s^2
  // - VECTOR_GRAVITY       - m/s^2
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);

  // BNO055のオイラー角は通常と異なり、X軸がHeading（方位角）に対応
  float heading = euler.x();

  return heading;
}
