#include <Arduino.h>
#include <Display.h>
#include <Avatar.h>

using namespace m5avatar;

Avatar avatar;

void setup()
{
  Display.begin();
  Display.setRotation(1);     // 画面回転(横向き)
//Display.setBrightness(255); // バックライト100%(全点灯)
  Display.fillScreen(TFT_BLACK);

  avatar.init(); // start drawing
}

void loop()
{
  //usleep(1000);
  delay(1);
}
