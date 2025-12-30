#include "SerialCom.h"

// 0x02/0x03だとデバッグしにくいので、#/$ を 電文開始/終了 に使う
// 電文開始
#define CODE_STX '#'
// 電文終了
#define CODE_ETX '$'

// 電文開始待ち状態
#define STATE_READY     0
// 電文受信中状態
#define STATE_RECEIVING 1

// 初期化
// serial: シリアルポート
// baud: ボーレート
// onReceived: 受信ハンドラ
void SerialCom::begin(Stream & serial, void (*onReceived)(char*))
{
    this->serial     = &serial;
    this->onReceived = onReceived;
    this->state      = STATE_READY;
    this->ptr        = 0;
}


// 受信ループ処理
void SerialCom::loop()
{
    char c;
    
    /* シリアル受信データがあるか？ */
    while (serial->available() > 0)
    {
        //serial->println("RECV ");
        c = serial->read();
        switch(state)
        {
        /* 電文開始待ち状態 */
        case STATE_READY:
            /* 電文開始コードが来たら電文受信中状態へ */
            if(c == CODE_STX)
            {
                //serial->println("STX ");
                state = STATE_RECEIVING;
                ptr = 0;
            }
            break;
        /* 電文受信中状態 */
        case STATE_RECEIVING:
            /* もしも電文開始コードが来たら受信中のデータを破棄 */
            if(c == CODE_STX)
            {
                //serial->println("STX ");
                ptr = 0;
            }
            /* 電文終了コードが来たら、受信した電文のコマンドを実行 */
            else if(c == CODE_ETX)
            {
                //serial->println("ETX ");
                buff[ptr] = '\0';
                this->onReceived(buff);
                state = STATE_READY;
            }
            /* 1文字受信 */
            else
            {
                buff[ptr] = c;
                ptr++;
                if(ptr>=RX_BUFF_SIZE)
                {
                    state = STATE_READY;
                }
            }
            break;
        default:
            state = STATE_READY;
        }
    }
}
