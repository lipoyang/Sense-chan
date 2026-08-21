#pragma once
#include <Arduino.h>

// BLEラジコン受信器
class SprReceiverBLE
{
public:
    void begin(Stream &serial);
    void loop();

    void (*onConnect)();
    void (*onDisconnect)();
    void (*onLost)();
    void (*onST)(int);
    void (*onTH)(int);
    void (*onSetMode)(int);

    uint32_t t_disconnect = 3000;
    uint32_t t_lost = 1000;
};
