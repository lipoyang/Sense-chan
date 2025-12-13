// BLEラジコン受信器のサンプルスケッチ
// SPRESENSEのオンボードLEDの輝度が変化します。
// LED0:前進, LED1:後退, LED2:左旋回, LED3:右旋回

#include <SprReceiverBLE.h>

// オンボードLEDの制御
static void led_set(int led_ch, int duty);
static void led_softwarePwm();

// BLEラジコン受信器
SprReceiverBLE receiver;

// 接続時
void onConnect()
{
  Serial.println("Connected!");
}

// 切断時
void onDisconnect()
{
  led_set(0, 0);
  led_set(1, 0);
  led_set(2, 0);
  led_set(3, 0);
  Serial.println("Disconnected!");
}

// 通信途絶時 (フェールセーフ処理)
void onLost()
{
  led_set(0, 0);
  led_set(1, 0);
  led_set(2, 0);
  led_set(3, 0);
  Serial.println("Lost!");
}

// スロットルコマンド受信時
// th : スロットル値 (-127～+127)
void onTH(int th)
{
  // 前進ならLED0, 後退ならLED1をPWM点灯
  if(th >= 0){
    led_set(0, th*2);
    led_set(1, 0);
  }else{
    led_set(0, 0);
    led_set(1, -th*2);
  }
  Serial.print("TH:");
  Serial.println(th);
}

// ステアリングコマンド受信時
// st : ステアリング値 (-127～+127)
void onST(int st)
{
  // 左旋回ならLED2, 右旋回ならLED3をPWM点灯
  if(st >= 0){
    led_set(2, st*2);
    led_set(3, 0);
  }else{
    led_set(2, 0);
    led_set(3, -st*2);
  }
  Serial.print("ST:");
  Serial.println(st);
}

// 初期化
void setup()
{
  Serial.begin(115200);
  
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  // BLEラジコン受信器の初期化
  receiver.onConnect = onConnect;
  receiver.onDisconnect = onDisconnect;
  receiver.onLost = onLost;
  receiver.onTH = onTH;
  receiver.onST = onST;
  receiver.begin();
}

// メインループ
void loop()
{
  // BLEラジコン受信器のメインループ処理
  receiver.loop();
  
  // オンボードLEDのソフトウェアPWM制御
  led_softwarePwm();
}

//****************************************
// オンボードLEDの制御
//****************************************
// SpresenseのオンボードLEDは analogWrite() ではPWM制御できないため
// ソフトウェアPWMで輝度を制御する。

#include <arch/board/board.h> // board_gpio_write()

// 各LEDの輝度 (0～255)
static int led_duty[4];

// LEDの輝度を設定する。
// led_ch : オンボードLEDの番号 (0～3)
// duty : 輝度 (0～255)
static void led_set(int led_ch, int duty)
{
  led_duty[led_ch] = duty;
}

// LEDのソフトウェアPWM出力
// loop()から呼び出す。loopではブロッキング処理をしないこと。
static void led_softwarePwm()
{
  static const int LED_PIN[4] = {
    PIN_I2S1_BCK,     // LED0
    PIN_I2S1_LRCK,    // LED1
    PIN_I2S1_DATA_IN, // LED2
    PIN_I2S1_DATA_OUT // LED3
  };

  bool led_on[4];
  for(int led = 0; led < 4; led++){
    if(led_duty[led] > 0){
      board_gpio_write(LED_PIN[led], HIGH);
      led_on[led] = true;
    }else{
      led_on[led] = false;
    }
  }

  for(int cnt = 0; cnt < 256; cnt++){
    for(int led = 0; led < 4; led++){
      if(led_on[led] && (cnt >= led_duty[led])){
        board_gpio_write(LED_PIN[led], LOW);
        led_on[led] = false;
      }
    }
    delayMicroseconds(10);
  }
}
