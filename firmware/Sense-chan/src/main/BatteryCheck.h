#pragma once

#include "PollingTimer.h"

// バッテリー電圧閾値
const float LOW_BATTERY = 3.55f; // 低バッテリー警告

// バッテリー電圧監視
class BatteryCheck
{
public:
    BatteryCheck() {};
    void begin();
    void loop();
    void (*onBatteryCheck)(float voltage, bool wasLowBattery) = nullptr;
private:
    IntervalTimer batteryTimer;
    bool wasLowBattery = false;
};