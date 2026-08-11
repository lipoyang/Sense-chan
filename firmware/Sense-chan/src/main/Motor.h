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
    };
    Parameter parameter = { 120.0f, 60.0f, 2.0f, 12.0f};

private:
    bool _pausing = false;
};