#pragma once

// モータの制御
class Motor
{
public:
    void begin();
    void loop();
    void setParameter();
    void setVelocityMode();
    void setPositionMode();
    void setVelocity(float l, float r);
    void setPosition(float x, float y);
    void setPause(bool pause);
    bool isPausing() const {return _pausing;}

    struct Parameter{
        float Kx;   // 旋回成分の係数 [度]
        float Ky;   // 並進成分の係数 [度]
        float Vmax; // // 位置制御の台形制御の最大速度
        float Vfull; // 速度制御の最大速度 [%]
        float Kp;   // PID制御の比例ゲイン
        float Ki;   // PID制御の積分ゲイン
        float Kd;   // PID制御の微分ゲイン
    };
    Parameter parameter = { 120.0f, 60.0f, 2.0f, 12.0f, 0.5f, 0.0f, 0.0f };

private:
    bool _pausing = false;
};