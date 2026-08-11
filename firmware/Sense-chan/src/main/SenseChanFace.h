#pragma once
#include <stdint.h>
#include <Avatar.h>

// 表情
using m5avatar::Expression;

// スタックチャンの顔表示器
class SenseChanFace
{
public:
    SenseChanFace() {};
    void begin();
    void loop();
    void setExpression(Expression expression, int duration_ms = 0);
    void setBaseExpression(Expression expression) {baseExpression = expression;}
    void setSpeachText(const char *text, int duration_ms = 0);
    void clearSpeachText();
    void setMicroMotion(bool enable) { isMicroMotion = enable; }
    bool isMicroMotionEnabled() const { return isMicroMotion; }

    // 微動用コールバック
    void (*onMicroMotion)(float x, float y) = nullptr;

    float pwmBL = 30.0f; // バックライトPWM (0.0～100.0)

private:
    bool isMicroMotion = false;
    uint32_t t0_expression;
    uint32_t t0_speech;
    uint32_t T_expression = 0;
    uint32_t T_speech = 0;
    Expression baseExpression = Expression::Neutral;
};