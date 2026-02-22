#include "SenseChanFace.h"
#include <MP.h>

// バックライト制御ピン
#define TFT_BL        9

// LCD表示処理用サブコアID
const int SUBCORE_LCD = 1;

// メッセージID定義
const int8_t MSGID_BEGUN = 1;
const int8_t MSGID_SET_BASE_EXPRESSION = 2;
const int8_t MSGID_SET_EXPRESSION = 3;
const int8_t MSGID_SET_SPEECH_TEXT = 4;
const int8_t MSGID_CLEAR_SPEECH_TEXT = 5;
const int8_t MSGID_MICRO_MOTION_ENABLE = 6;
const int8_t MSGID_MICRO_MOTION_DISABLE = 7;
const int8_t MSGID_MICRO_MOTION = 8;

// スタックチャンの顔表示の初期化
void SenseChanFace::begin()
{
    // バックライトOFF
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);

    // LCD表示処理用のサブコア起動
    int ret = MP.begin(SUBCORE_LCD);
    if (ret < 0) {
        Serial.printf("SenseChanFace: MP.begin error = %d\n", ret);
    }
    // サブコアの起動完了待ち
    MP.RecvTimeout(MP_RECV_BLOCKING);
    int8_t msgid;
    uint32_t dummy = 0;
    MP.Recv(&msgid, &dummy, SUBCORE_LCD);
    if (msgid != MSGID_BEGUN) {
        Serial.printf("SenseChanFace: MP.Recv error: no BEGUN message %d\n", msgid);
    }
    // 受信をポーリングに変更
    MP.RecvTimeout(MP_RECV_POLLING);
}

// スタックチャンの顔表示のメインループ処理
void SenseChanFace::loop()
{
    // メッセージID
    int8_t msgid;
    // メッセージデータ
    static struct {
        float x;
        float y;
    } *msgdata;

    int ret = MP.Recv(&msgid, &msgdata, SUBCORE_LCD);
    if(ret == -EAGAIN) {
        return; // 受信無し
    }
    if(ret < 0) {
        Serial.printf("SenseChanFace: MP.Recv error %d\n", ret);
        return;
    }
    
    switch (ret) {
    case MSGID_MICRO_MOTION:
      if (onMicroMotion) {
          onMicroMotion(msgdata->x, msgdata->y);
      }
      break;
    }
}

// スタックチャンの表情を設定
void SenseChanFace::setBaseExpression(Expression expression)
{
    // メッセージ送信
    static struct {
        uint32_t expression;
    } msgdata;
    msgdata.expression = (uint32_t)expression;
    MP.Send(MSGID_SET_BASE_EXPRESSION, &msgdata, SUBCORE_LCD);
}

// スタックチャンの表情を設定
void SenseChanFace::setExpression(Expression expression, int duration_ms)
{
    // メッセージ送信
    static struct {
        uint32_t expression;
        uint32_t duration_ms;
    } msgdata;
    msgdata.expression = (uint32_t)expression;
    msgdata.duration_ms = (uint32_t)duration_ms;
    MP.Send(MSGID_SET_EXPRESSION, &msgdata, SUBCORE_LCD);
}

// スタックチャンのセリフを設定
void SenseChanFace::setSpeachText(const char *text, int duration_ms)
{
    // テキストデータのコピー
    static char text_buffer[256];
    strncpy(text_buffer, text, sizeof(text_buffer) - 1);
    text_buffer[sizeof(text_buffer) - 1] = '\0';
    
    // メッセージ送信
    static struct {
        const char *text;
        uint32_t duration_ms;
    } msgdata;
    msgdata.text = text_buffer;
    msgdata.duration_ms = (uint32_t)duration_ms;
    MP.Send(MSGID_SET_SPEECH_TEXT, &msgdata, SUBCORE_LCD);
}

// スタックチャンのセリフをクリア
void SenseChanFace::clearSpeachText()
{
    // メッセージ送信
    uint32_t dummy = 0;
    MP.Send(MSGID_CLEAR_SPEECH_TEXT, dummy, SUBCORE_LCD);
}

// スタックチャンの微動を有効/無効にする
void SenseChanFace::setMicroMotion(bool enabled) {
    // メッセージ送信
    uint32_t dummy = (uint32_t)enabled;
    int8_t msgid = enabled ? MSGID_MICRO_MOTION_ENABLE : MSGID_MICRO_MOTION_DISABLE;
    MP.Send(msgid, dummy, SUBCORE_LCD);
}
