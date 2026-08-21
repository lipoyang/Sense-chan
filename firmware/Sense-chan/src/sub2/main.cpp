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
const int8_t MSGID_SET_PARAMS = 0;
const int8_t MSGID_BEGUN = 1;
const int8_t MSGID_CONNECT = 2;
const int8_t MSGID_DISCONNECT = 3;
const int8_t MSGID_RECEIVE = 4;
const int8_t MSGID_SET_MODE = 5;
const int8_t MSGID_VOLTAGE = 6;

// ソフトウェアシリアルのピン
SoftwareSerial *softSerial;

// BLEラジコン受信器
SprReceiverBLE receiver;
static int g_fb = 0;  // 前後方向
static int g_lr = 0;  // 左右方向
static int g_mode = 0; // モード

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
void propo_message()
{
  int32_t l = (int32_t)(g_fb - g_lr/2);
  int32_t r = (int32_t)(g_fb + g_lr/2);
  // -127~+127にクリップ
  if (l < -127) l = -127;
  if (l > +127) l = +127;
  if (r < -127) r = -127;
  if (r > +127) r = +127;

  // メッセージデータ
  static struct {
      int32_t l;
      int32_t r;
  } msgdata;
  msgdata.l = l;
  msgdata.r = r;

  MP.Send(MSGID_RECEIVE, &msgdata, MAINCORE_ID);
}

// モードメッセージをメインコアへ送る
void mode_message()
{
  // メッセージデータ
  static struct {
      int32_t mode;
  } msgdata;
  msgdata.mode = g_mode;

  MP.Send(MSGID_SET_MODE, &msgdata, MAINCORE_ID);
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
  propo_message();
}

// スロットルコマンド受信時
// th : スロットル値 (-127～+127)
void onTH(int th)
{
  g_fb = th;
  propo_message();
}

// ステアリングコマンド受信時
// st : ステアリング値 (-127～+127)
void onST(int st)
{
  g_lr = st;
  propo_message();
}

// モード設定コマンド受信時
// mode : モード値
void onSetMode(int mode)
{
  g_mode = mode;
  mode_message();
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
  MP.RecvTimeout(MP_RECV_BLOCKING);
  MP.Send(MSGID_BEGUN, dummy, MAINCORE_ID);

  // SoftwareSerialのパラメータをメインコアから受信
  int8_t msgid;
  struct SoftSerialParams {
    int RX;
    int TX;
    int Baud;
  } *softSerialParams;
  do {
    MP.Recv(&msgid, &softSerialParams);
    if(msgid == MSGID_SET_PARAMS){
      break;
    }else{
      MPLog("Unexpected message %d\n", msgid);
    }
  } while(1);

  // BLEラジコン受信器の初期化
  softSerial = new SoftwareSerial(softSerialParams->RX, softSerialParams->TX);
  softSerial->begin(softSerialParams->Baud);
  receiver.onConnect = onConnect;
  receiver.onDisconnect = onDisconnect;
  receiver.onLost = onLost;
  receiver.onTH = onTH;
  receiver.onST = onST;
  receiver.onSetMode = onSetMode;
  receiver.begin(*softSerial);
  
  // 設定完了をメインコアに知らせる
  MP.Send(MSGID_SET_PARAMS, dummy, MAINCORE_ID);
  MP.RecvTimeout(MP_RECV_POLLING);
}

// メインループ
void loop()
{
  // BLEラジコン受信器のメインループ処理
  receiver.loop();

  // メッセージID
  int8_t msgid;

  // メッセージデータ
  typedef struct {
      int32_t voltage;
  } S_Voltage;

  typedef union {
    S_Voltage voltage;
  } MsgData;
  MsgData *msgdata;
  uint32_t udata;

  // メッセージ受信
  int ret = MP.Recv(&msgid, &msgdata);
  switch(ret){
    case MSGID_VOLTAGE:
    {
      int32_t voltage_mv = msgdata->voltage.voltage;
      
      static char buff[10];
      sprintf(buff, "#B%04X$\r\n", voltage_mv);
      softSerial->print(buff);
      // MPLog("%s\n", buff);      
      break;
    }
  }
}
