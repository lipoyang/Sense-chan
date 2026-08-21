#include "BleReceiver.h"
#include <MP.h>

// BLE通信処理用サブコアID
const int SUBCORE_BLE = 2;

// メッセージID定義
const int8_t MSGID_SET_PARAMS = 0;
const int8_t MSGID_BEGUN = 1;
const int8_t MSGID_CONNECT = 2;
const int8_t MSGID_DISCONNECT = 3;
const int8_t MSGID_RECEIVE = 4;
const int8_t MSGID_SET_MODE = 5;

// 受信器の初期化
void BleReceiver::begin()
{
    // LCD表示処理用のサブコア起動
    int ret = MP.begin(SUBCORE_BLE);
    if (ret < 0) {
        Serial.printf("BleReceiver: MP.begin error = %d\n", ret);
    }
    // サブコアの起動完了待ち
    MP.RecvTimeout(MP_RECV_BLOCKING);
    int8_t msgid;
    uint32_t dummy = 0;
    MP.Recv(&msgid, &dummy, SUBCORE_BLE);
    if (msgid != MSGID_BEGUN) {
        Serial.printf("BleReceiver: MP.Recv error: no BEGUN message %d\n", msgid);
    }

    // サブコアにシリアル通信パラメータを送信
    struct SoftSerialParams {
        int RX;
        int TX;
        int Baud;
    } softSerialParams;
    softSerialParams.RX = RX;
    softSerialParams.TX = TX;
    softSerialParams.Baud = Baud;
    MP.Send(MSGID_SET_PARAMS, &softSerialParams, SUBCORE_BLE);
    MP.Recv(&msgid, &dummy, SUBCORE_BLE);
    if (msgid != MSGID_SET_PARAMS) {
        Serial.printf("BleReceiver: MP.Recv error: no SET_PARAMS message %d\n", msgid);
    }

    // 受信をポーリングに変更
    MP.RecvTimeout(MP_RECV_POLLING);
}

// 受信器のメインループ処理
void BleReceiver::loop()
{
    // メッセージID
    int8_t msgid;

    // メッセージデータ
    typedef struct {
        int32_t l;
        int32_t r;
    } S_Receive;
    typedef struct {
        int32_t mode;
    } S_SetMode;

    typedef union {
        S_Receive receiveData;
        S_SetMode setModeData;
    } MsgData;
    MsgData *msgdata;

    int ret = MP.Recv(&msgid, &msgdata, SUBCORE_BLE);
    if(ret == -EAGAIN) {
        return; // 受信無し
    }
    if(ret < 0) {
        Serial.printf("BleReceiver: MP.Recv error %d\n", ret);
        return;
    }
    
    switch (ret) {
    case MSGID_CONNECT:
        if (onConnect) {
            onConnect();
        }
        break;
    case MSGID_DISCONNECT:
        if (onDisconnect) {
            onDisconnect();
        }
        break;
    case MSGID_RECEIVE:
        if (onReceive) {
            onReceive(
                msgdata->receiveData.l,
                msgdata->receiveData.r
            );
        }
        break;
    case MSGID_SET_MODE:
        if (onSetMode) {
            onSetMode(
                msgdata->setModeData.mode
            );
        }
    default:
        Serial.printf("BleReceiver: unknown msgid %d\n", msgid);
        break;
    }
}
