#pragma once

#define RX_BUFFER_SIZE 128
#define RX_MAX_ARGS 10

// USBシリアルコマンドクラス
class SerialCmd
{
public:
    void begin();
    void loop();

    void (*onCommand)(int argc, char const *argv[]) = nullptr;
private:
    int rxIndex;
    char rxBuff[RX_BUFFER_SIZE];
    
    char *argv[RX_MAX_ARGS];
    int argc;
};