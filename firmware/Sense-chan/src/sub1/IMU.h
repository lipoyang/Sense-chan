#pragma once

class IMU {
public:
    bool begin();
    void update();
    void calibrate() { calibCnt = 0; g0 = 0.0f; }; // キャリブレーション開始
    inline void setCalib(float offset) { this->g0 = offset; };
    inline float getTheta() const { return theta; };
    inline float getGyro() const { return gyro; };
    inline bool isError() const { return _isError; }; 
private:
    float gyro;     // 方位角の角速度
    float theta;    // 方位角
    float g0;       // ジャイロの静止オフセット (方位角成分)
    int calibCnt;   // キャリブレーションカウンタ
    bool _isError = false;
};