#include <Arduino.h>
#include "SerialCmd.h"

// USBシリアルコマンドの初期化
void SerialCmd::begin()
{
//    rxState = STATE_READY;
    Serial.begin(115200);
    rxIndex = 0;
}

// USBシリアルコマンドのメインループ処理
void SerialCmd::loop()
{
    while (Serial.available())
    {
        char c = Serial.read();
        if(c == '\n' || c == '\r')
        {
            if(rxIndex == 0) continue;
            rxBuff[rxIndex] = '\0';
            rxIndex = 0;

            // コマンドと引数を分割
            argc = 0;
            char *token = strtok(rxBuff, " ");
            while (token != NULL && argc < RX_MAX_ARGS) {
                argv[argc] = token;
                argc++;
                token = strtok(NULL, " ");
            }
            // コマンドコールバックの呼び出し
            if (onCommand != nullptr) {
                onCommand(argc, (const char**)argv);
            }
        }
        else
        {
            rxBuff[rxIndex] = c;
            rxIndex++;
            if(rxIndex >= RX_BUFFER_SIZE)
            {
                rxIndex = 0;
            }
        }
    }
}
