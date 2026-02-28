#pragma once

// SDカードクラス
class SdCard
{
public:
    bool begin();
    bool load();
    bool save();

    float Kx = 120.0f;
    float Ky = 60.0f;
    float Vmax = 2.0f;
};