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
    int RX = 25;
    int TX = 26;
    int Baud = 115200;
    float PWM = 30.0f;
};