#pragma once

#include <Avatar.h>

class SenseChanFace
{
public:
    SenseChanFace() {};
    void begin();
    void loop();
    void setExpression(m5avatar::Expression expression, int duration_ms = 0);
    void setBaseExpression(m5avatar::Expression expression) {baseExpression = expression;}
    void setSpeachText(const char *text, int duration_ms = 0);
    void clearSpeachText();

private:
    bool isMicroMotion = false;
    uint32_t t0_expression;
    uint32_t t0_speech;
    uint32_t T_expression = 0;
    uint32_t T_speech = 0;
    m5avatar::Expression baseExpression = m5avatar::Expression::Neutral;
};