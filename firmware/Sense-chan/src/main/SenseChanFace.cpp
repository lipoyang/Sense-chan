#include "SenseChanFace.h"
#include <MP.h>

// LCD表示処理用サブコアID
const int SUBCORE_LCD = 1;

// メッセージID定義
const int8_t MSGID_BEGUN = 1;
const int8_t MSGID_SET_BASE_EXPRESSION = 2;
const int8_t MSGID_SET_EXPRESSION = 3;
const int8_t MSGID_SET_SPEECH_TEXT = 4;
const int8_t MSGID_CLEAR_SPEECH_TEXT = 5;

// スタックチャン頭部の初期化
void SenseChanFace::begin()
{
    // LCD表示処理用のサブコア起動
    int ret = MP.begin(SUBCORE_LCD);
    if (ret < 0) {
        Serial.printf("MP.begin error = %d\n", ret);
    }
    // サブコアの起動完了待ち
    MP.RecvTimeout(MP_RECV_BLOCKING);
    int8_t msgid;
    int *dummy;
    MP.Recv(&msgid, dummy, SUBCORE_LCD);
    if (msgid != MSGID_BEGUN) {
        Serial.printf("MP.Recv error: no BEGUN message %d\n", msgid);
    }
}

// スタックチャンの表情を設定
void setBaseExpression(Expression expression)
{
    // メッセージ送信
    MP.Send(MSGID_SET_BASE_EXPRESSION, (uint32_t)expression, SUBCORE_LCD);
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
