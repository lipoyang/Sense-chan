#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

#include <Arduino.h>
#include <MP.h>
#include "SenseChanFace.h"

using namespace m5avatar;

// バックライト制御ピン
#define TFT_BL        9

// メインコアID
const int MAINCORE_ID = 0;

// メッセージID定義
const int8_t MSGID_BEGUN = 1;
const int8_t MSGID_SET_BASE_EXPRESSION = 2;
const int8_t MSGID_SET_EXPRESSION = 3;
const int8_t MSGID_SET_SPEECH_TEXT = 4;
const int8_t MSGID_CLEAR_SPEECH_TEXT = 5;

// 顔表示オブジェクト
SenseChanFace face;

// エラーループ
void errorLoop(int num)
{
  int i;
  while (1) {
    for (i = 0; i < num; i++) {
      ledOn(LED1);
      delay(300);
      ledOff(LED1);
      delay(300);
    }
    delay(1000);
  }
}

// 初期化
void setup()
{
  // サブコア開始
  int ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }

  // LCD初期化
  Display.begin();
  Display.setRotation(3);     // 画面回転(横向き,反転)
  //Display.setBrightness(255); // バックライト100%(全点灯)
  Display.fillScreen(TFT_BLACK);

  // バックライトON
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 顔の初期化
  face.begin(); 

  // 初期化完了をメインコアに知らせる
  uint32_t dummy = 0;
  MP.Send(MSGID_BEGUN, dummy, MAINCORE_ID);
  MP.RecvTimeout(MP_RECV_POLLING);
}

// メインループ
void loop()
{
  // メッセージID
  int8_t msgid;

  // メッセージデータ
  typedef struct {
    uint32_t expression;
    uint32_t duration_ms;
  } S_Expression;

  typedef struct {
    const char *text;
    uint32_t duration_ms;
  } S_SpeechText;

  typedef union {
    S_Expression expression;
    S_SpeechText speechText;
    uint32_t     baseExpression;
  } MsgData;
  MsgData *msgdata;

  // メッセージ受信
  int ret = MP.Recv(&msgid, &msgdata);
  switch(ret){
    case MSGID_SET_BASE_EXPRESSION:
      face.setBaseExpression(
          (m5avatar::Expression)(msgdata->baseExpression));
      break;
    case MSGID_SET_EXPRESSION:
      face.setExpression(
          (m5avatar::Expression)(msgdata->expression.expression),
          (int)(msgdata->expression.duration_ms));
      break;
    case MSGID_SET_SPEECH_TEXT:
      face.setSpeachText(
          msgdata->speechText.text,
          (int)(msgdata->speechText.duration_ms));
      break;
    case MSGID_CLEAR_SPEECH_TEXT:
      face.clearSpeachText();
      break;
  }

  // 顔表示処理
  face.loop();
}
