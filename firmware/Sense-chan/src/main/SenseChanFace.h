#pragma once
#include <stdint.h>

// 表情
enum class Expression { Happy, Angry, Sad, Doubt, Sleepy, Neutral };

// スタックチャンの顔表示器
class SenseChanFace
{
public:
    SenseChanFace() {};
    void begin();
    void loop();
    void setExpression(Expression expression, int duration_ms = 0);
    void setBaseExpression(Expression expression);
    void setSpeachText(const char *text, int duration_ms = 0);
    void clearSpeachText();
    void setMicroMotion(bool enabled);

    // 微動用コールバック
    void (*onMicroMotion)(float x, float y) = nullptr;
};