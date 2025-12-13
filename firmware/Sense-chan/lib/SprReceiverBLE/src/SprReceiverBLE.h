#pragma once

// BLEラジコン受信器
class SprReceiverBLE
{
public:
    void begin();
    void loop();

    void (*onConnect)();
    void (*onDisconnect)();
    void (*onLost)();
    void (*onST)(int);
    void (*onTH)(int);

    uint32_t t_disconnect = 3000;
    uint32_t t_lost = 1000;
};
