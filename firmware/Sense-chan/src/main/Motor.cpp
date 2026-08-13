#include "Motor.h"
#include <MP.h>

// モータ制御用サブコアID
const int SUBCORE_MOTOR = 1;

// メッセージID定義
const int8_t MSGID_BEGUN = 1;
const int8_t MSGID_SET_PARAMETER = 2;
const int8_t MSGID_SET_VELOCITY_MODE = 3;
const int8_t MSGID_SET_POSITION_MODE = 4;
const int8_t MSGID_SET_VELOCITY = 5;
const int8_t MSGID_SET_POSITION = 6;
const int8_t MSGID_SET_PAUSE = 7;

// 初期化
void Motor::begin()
{
    // モータ制御用のサブコア起動
    int ret = MP.begin(SUBCORE_MOTOR);
    if (ret < 0) {
        Serial.printf("Motor: MP.begin error = %d\n", ret);
    }
    // サブコアの起動完了待ち
    MP.RecvTimeout(MP_RECV_BLOCKING);
    int8_t msgid;
    uint32_t dummy = 0;
    MP.Recv(&msgid, &dummy, SUBCORE_MOTOR);
    if (msgid != MSGID_BEGUN) {
        Serial.printf("Motor: MP.Recv error: no BEGUN message %d\n", msgid);
    }

    // パラメータ設定
    MP.Send(MSGID_SET_PARAMETER, &parameter, SUBCORE_MOTOR);
    MP.Recv(&msgid, &dummy, SUBCORE_MOTOR);
    if (msgid != MSGID_SET_PARAMETER) {
        Serial.printf("Motor: MP.Recv error: no SET_PARAMETER message %d\n", msgid);
    }

    // 受信をポーリングに変更
    MP.RecvTimeout(MP_RECV_POLLING);
}

// メインループ処理
void Motor::loop()
{
    ; // 現状は特に何もしない
}

// 制御パラメータを設定
void Motor::setParameter()
{
    MP.Send(MSGID_SET_PARAMETER, &parameter, SUBCORE_MOTOR);
}

// 速度制御モードに設定
void Motor::setVelocityMode()
{
    uint32_t dummy = 0;
    MP.Send(MSGID_SET_VELOCITY_MODE, dummy, SUBCORE_MOTOR);
}

// 位置制御モードに設定
void Motor::setPositionMode()
{
    uint32_t dummy = 0;
    MP.Send(MSGID_SET_POSITION_MODE, dummy, SUBCORE_MOTOR);
}

// 速度の目標値を設定
void Motor::setVelocity(float l, float r)
{
    static struct {
        float l;
        float r;
    } msgdata;
    msgdata.l = l;
    msgdata.r = r;
    MP.Send(MSGID_SET_VELOCITY, &msgdata, SUBCORE_MOTOR);
}

// 位置の目標値を設定
void Motor::setPosition(float x, float y)
{
    static struct {
        float x;
        float y;
    } msgdata;
    msgdata.x = x;
    msgdata.y = y;
    MP.Send(MSGID_SET_POSITION, &msgdata, SUBCORE_MOTOR);
}

// 一時停止する
// pause : true:一時停止 / false:解除
void Motor::setPause(bool pause)
{
    static struct {
        bool pause;
    } msgdata;
    msgdata.pause = pause;

    MP.Send(MSGID_SET_PAUSE, &msgdata, SUBCORE_MOTOR);
    _pausing = pause;
}
