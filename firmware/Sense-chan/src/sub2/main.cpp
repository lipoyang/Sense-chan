#if (SUBCORE != 2)
#error "Core selection is wrong!!"
#endif

#include <Arduino.h>
#include <MP.h>
#include <SoftwareSerial.h>
#include <SprReceiverBLE.h>

// メインコアID
const int MAINCORE_ID = 0;

// メッセージID定義
const int8_t MSGID_BEGUN = 1;
const int8_t MSGID_CONNECT = 2;
const int8_t MSGID_DISCONNECT = 3;
const int8_t MSGID_RECEIVE = 4;

// ソフトウェアシリアルのピン
#define PIN_RX 2
#define PIN_TX 4
SoftwareSerial softSerial(PIN_RX, PIN_TX);

// BLEラジコン受信器
SprReceiverBLE receiver;
static int g_fb = 0;  // 前後方向
static int g_lr = 0;  // 左右方向

// エラーループ
void errorLoop(int num)
{
  int i;
  while (1) {
    for (i = 0; i < num; i++) {
      ledOn(LED2);
      delay(300);
      ledOff(LED2);
      delay(300);
    }
    delay(1000);
  }
}

// プロポメッセージをメインコアへ送る
void propo_massage()
{
  // メッセージデータ
  static struct {
      int32_t l;
      int32_t r;
  } msgdata;

  msgdata.l = (int32_t)(g_fb - g_lr/2);
  msgdata.r = (int32_t)(g_fb + g_lr/2);
  msgdata.l /= 2;
  msgdata.r /= 2;

  MP.Send(MSGID_RECEIVE, &msgdata, MAINCORE_ID);
}

// 接続時
void onConnect()
{
  uint32_t dummy = 0;
  MP.Send(MSGID_CONNECT, dummy, MAINCORE_ID);
}

// 切断時
void onDisconnect()
{
  uint32_t dummy = 0;
  MP.Send(MSGID_DISCONNECT, dummy, MAINCORE_ID);
}

// 通信途絶時 (フェールセーフ処理)
void onLost()
{
  g_fb = 0;
  g_lr = 0;
  propo_massage();
}

// スロットルコマンド受信時
// th : スロットル値 (-127～+127)
void onTH(int th)
{
  g_fb = th;
  propo_massage();
}

// ステアリングコマンド受信時
// st : ステアリング値 (-127～+127)
void onST(int st)
{
  g_lr = st;
  propo_massage();
}

// 初期化
void setup()
{
  // サブコア開始
  int ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }
  uint32_t dummy = 0;
  MP.Send(MSGID_BEGUN, dummy, MAINCORE_ID);
  MP.RecvTimeout(MP_RECV_POLLING);

  // BLEラジコン受信器の初期化
  softSerial.begin(19200);
  receiver.onConnect = onConnect;
  receiver.onDisconnect = onDisconnect;
  receiver.onLost = onLost;
  receiver.onTH = onTH;
  receiver.onST = onST;
  receiver.begin(softSerial);
}

// メインループ
void loop()
{
  // BLEラジコン受信器のメインループ処理
  receiver.loop();
}
