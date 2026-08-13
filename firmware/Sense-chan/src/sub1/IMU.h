#pragma once

class IMU {
public:
    bool begin();
    bool isCalibrated();
    int getTemperature();
    float getHeading();
};