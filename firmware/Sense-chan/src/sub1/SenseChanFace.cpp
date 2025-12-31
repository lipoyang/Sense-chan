#include "SenseChanFace.h"

using namespace m5avatar;
static Avatar avatar;

// スタックチャンの顔表示の初期化
void SenseChanFace::begin()
{
    // アバターの初期化
    avatar.init();
//  avatar.addTask(task_servo, "servo");
    avatar.setSpeechFont(&fonts::efontJA_16);
    avatar.setSpeechText("こんにちは");
    delay(2000);
    avatar.setSpeechText("");
}

// スタックチャン頭部のループ処理
void SenseChanFace::loop()
{
    uint32_t now = millis();

    if (T_expression > 0 && (now - t0_expression) >= T_expression) {
        avatar.setExpression(baseExpression);
        T_expression = 0;
    }

    if (T_speech > 0 && (now - t0_speech) >= T_speech) {
        avatar.setSpeechText("");
        T_speech = 0;
    }
}

// スタックチャンの表情を設定
void SenseChanFace::setExpression(Expression expression, int duration_ms)
{
    avatar.setExpression(expression);
    if (duration_ms > 0) {
        t0_expression = millis();
        T_expression = duration_ms;
    }
}

// スタックチャンのセリフを設定
void SenseChanFace::setSpeachText(const char *text, int duration_ms)
{
    avatar.setSpeechText(text);
    if (duration_ms > 0) {
        t0_speech = millis();
        T_speech = duration_ms;
    }
}

// スタックチャンのセリフをクリア
void SenseChanFace::clearSpeachText()
{
    avatar.setSpeechText("");
}
