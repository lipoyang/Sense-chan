#include <Arduino.h>
#include "BatteryCheck.h"

// 電源監視ピン
#define POWER_MONITOR_PIN A0 // AIN2であることに注意

// バッテリー電圧監視の初期化
void BatteryCheck::begin()
{
    wasLowBattery = false;

    // タイマーの設定
    batteryTimer.set(1000); // 1秒ごとに監視
}

// バッテリー電圧監視のメインループ処理
void BatteryCheck::loop()
{
    // タイマーが経過したら
    if(batteryTimer.elapsed()) {
        // バッテリー電圧の測定
        int vbat_raw = analogRead(POWER_MONITOR_PIN);
        float vbat = vbat_raw  * 5.04f / 1023.0f * 1.042; // 1.042 is 何？
        // Serial.printf("Power Voltage: %.2f V\n", vbat);

        // コールバックの呼び出し
        if(onBatteryCheck != nullptr) {
            onBatteryCheck(vbat, wasLowBattery);
        }
        // 低バッテリー状態の更新
        if(vbat < LOW_BATTERY) {
            wasLowBattery = true;
        } else {
            wasLowBattery = false;
        }   
    }
}