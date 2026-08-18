#pragma once

// SDカードクラス
class SdCard
{
public:
    bool begin();
    bool load();
    bool save();

    float Kx = 60.0f;
    float Ky = 60.0f;
    float Vmax = 2.0f;
    float Vfull = 12.0f;
    float Kp = 0.5f;
    float Ki = 0.0f;
    float Kd = 0.0f;
    int RX = 25;
    int TX = 26;
    int Baud = 115200;
    float PWM = 30.0f;
};