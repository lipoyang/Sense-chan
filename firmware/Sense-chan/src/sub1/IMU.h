#pragma once

class IMU {
public:
    bool begin();
    void update();
    void calibrate() { calibCnt = 0; g0 = 0.0f; }; // キャリブレーション開始
    inline void setCalib(float offset) { this->g0 = offset; };
    inline float getHeading() const { return theta; };
private:
    float theta; // 方位角
    float g0; // ジャイロの静止オフセット
    int calibCnt; // キャリブレーションカウンタ
};