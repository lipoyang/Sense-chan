#pragma once

// BLEラジコン受信器
class BleReceiver
{
public:
    BleReceiver() {};
    void begin();
    void loop();
    void (*onConnect)();
    void (*onDisconnect)();
    void (*onReceive)(int l, int r);
};