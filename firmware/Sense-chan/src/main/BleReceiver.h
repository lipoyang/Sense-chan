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
    void (*onSetMode)(int mode);

    int RX = 25;   // RXピン番号
    int TX = 26;   // TXピン番号
    int Baud = 115200; // シリアル通信速度
};