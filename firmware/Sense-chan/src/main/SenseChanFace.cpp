#include "SenseChanFace.h"
#include "FS_U8g2font.h"

// バックライト制御ピン
#define TFT_BL        9

using namespace m5avatar;
static Avatar avatar;
static SenseChanFace *face;
static lgfx::FS_U8g2font font; // ファイルシステム版フォント

// 微動用コールバック
static void* task_servo(void *args)
{
  float gazeX, gazeY;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
    if(face->isMicroMotionEnabled())
    {
      avatar->getGaze(&gazeY, &gazeX);

      if(face->onMicroMotion != nullptr) {
        face->onMicroMotion(gazeX, gazeY);
      }
    }
    usleep(50*1000);
  }
}

// スタックチャンの顔表示の初期化
void SenseChanFace::begin()
{
    // バックライトOFF
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);

    // LCD初期化
    Display.begin();
    Display.setRotation(3);     // 画面回転(横向き,反転)
    //Display.setBrightness(255); // バックライト100%(全点灯)
    Display.fillScreen(TFT_BLACK);

    // バックライトON
    // digitalWrite(TFT_BL, HIGH);
    analogWrite(TFT_BL, 128); // PWM 50%

    if(font.loadFont("/mnt/sd0/fs_efont_ja_16.bin") != true)
    {
        Serial.println("Font load error!");
    }

    face = this;

    // アバターの初期化
    avatar.init();
    avatar.addTask(task_servo, "servo");  // 微動用コールバックの設定
//  avatar.setSpeechFont(&fonts::efontJA_16);
    avatar.setSpeechFont(&font);
    avatar.setSpeechText("こんにちは");
    delay(2000);
    avatar.setSpeechText("");
}

// スタックチャンの顔表示のメインループ処理
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
    font.loadGlyphCache(text);
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
