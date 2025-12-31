#pragma once

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