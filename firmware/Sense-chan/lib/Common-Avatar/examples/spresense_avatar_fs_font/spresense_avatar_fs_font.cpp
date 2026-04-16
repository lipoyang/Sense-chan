#include <Arduino.h>
#include <Display.h>
#include <Avatar.h>

#include <SDHCI.h>
#include "FS_U8g2font.h"
SDClass SD;             // SDカード
lgfx::FS_U8g2font font; // ファイルシステム版フォント
int sdcard_reset();     // SDカードリセット (自作基板の設計ミスのため)

#define TFT_BL        9 // バックライトのピン

using namespace m5avatar;

Avatar avatar;

void setup()
{
  Display.begin();
  Display.setRotation(3);     // 画面回転(横向き)
//Display.setBrightness(255); // バックライト100%(全点灯)
  Display.fillScreen(TFT_BLACK);

  Serial.begin(115200);
// while(!Serial){;}

  int ret = sdcard_reset();
  if(ret < 0) {
      Serial.println("SD card reset failed");
  }

  if(font.loadFont("/mnt/sd0/fs_efont_ja_16.bin") != true)
  {
    Serial.println("Font load error!");
  }
  font.loadGlyph("国破山河在");

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // バックライトON

  avatar.init(); // start drawing

  avatar.setSpeechFont(&font);
  
  avatar.setSpeechText("こんにちは");
}

// 動的にロードする文字列
const char* strdata[]={
  "葡萄美酒夜光杯",
  "欲飲琵琶馬上催",
  "酔臥沙場君莫笑",
  "古来征戦幾人回"
};
int cnt = 0;

void loop()
{
  usleep(1000*1000);
  avatar.setSpeechText("Hello!");

  usleep(1000*1000);
  avatar.setSpeechText("国破山河在");

  usleep(1000*1000);
  avatar.setSpeechText("こんにちは");

  usleep(1000*1000);
  font.loadGlyphCache(strdata[cnt]);
  avatar.setSpeechText(strdata[cnt]);
  cnt = (cnt + 1) % 4;
}
