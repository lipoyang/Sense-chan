#include <Arduino.h>
#include <Display.h>
#include <Avatar.h>

using namespace m5avatar;

#define TFT_BL        9

Avatar avatar;

void setup()
{
  Display.begin();
  Display.setRotation(1);     // 画面回転(横向き)
//Display.setBrightness(255); // バックライト100%(全点灯)
  Display.fillScreen(TFT_BLACK);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // バックライトON

  avatar.init(); // start drawing
}

void loop()
{
  //usleep(1000);
  delay(1);
}
