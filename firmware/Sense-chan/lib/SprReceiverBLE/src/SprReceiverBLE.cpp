#include <Arduino.h>
#include "SprReceiverBLE.h"
#include "utility/SerialCom.h"
#include "utility/AsciiInt.h"

// ラジコン受信機
static SprReceiverBLE *receiver;
// シリアル受信受信クラス
static SerialCom serialCom;

static bool is_connected = false;   // 接続フラグ
static uint32_t t_last_recv;        // 最終受信時刻
static bool is_driving = false;     // 走行中フラグ
static uint32_t t_last_recv2;       // 走行コマンドの最終受信時刻
static int st;  // ステアリング値
static int th;  // スロットル値

// 切断チェック
static void disconnect_check()
{
  uint32_t t_now = millis();

  if(is_connected){
    uint32_t t_elasped = t_now - t_last_recv;
    if(t_elasped > receiver->t_disconnect){
      is_connected = false;
      if(receiver->onDisconnect != nullptr){
        receiver->onDisconnect();
      }
    }
  }
  if(is_driving){
    uint32_t t_elasped = t_now - t_last_recv2;
    if(t_elasped > receiver->t_lost){
      is_driving = false;
      if(receiver->onLost != nullptr){
        receiver->onLost();
      }
    }
  }
}

// 受信したコマンドの実行
// buff: 受信したコマンドへのポインタ
static void onReceived(char* buff)
{
    unsigned short val;
    
    if(!is_connected){
      is_connected = true;
      if(receiver->onConnect != nullptr){
        receiver->onConnect();
      }
    }
    t_last_recv = millis();
    
    // Serial.println(buff); // TEST
    
    switch(buff[0])
    {
    /* Dコマンド(スロットル)
       書式: #Dxx$
       xx: 0のとき停止、正のとき前進、負のとき後退。
     */
    case 'D':
        // 値の解釈
        if( HexToUint16(&buff[1], &val, 2) != 0 ) break;
        th = (int)((signed char)val);
        // イベント
        if(receiver->onTH != nullptr){
          receiver->onTH(th);
        }
        // 走行中判定
        if((th == 0) && (st == 0)){
          is_driving = false;
        }else{
          is_driving = true;
          t_last_recv2 = millis();
        }
        break;
        
    /* Tコマンド(ステアリング)
       書式: #Txx$
       xx: 0のとき中立、正のとき右旋回、負のとき左旋回
     */
    case 'T':
        // 値の解釈
        if( HexToUint16(&buff[1], &val, 2) != 0 ) break;
        st = (int)((signed char)val);
        // イベント
        if(receiver->onST != nullptr){
          receiver->onST(st);
        }
        // 走行中判定
        if((th == 0) && (st == 0)){
          is_driving = false;
        }else{
          is_driving = true;
          t_last_recv2 = millis();
        }
        break;
    }
}

// 初期化
void SprReceiverBLE::begin(Stream &serial)
{
  receiver = this;

  // シリアルコマンド受信の初期化
  serialCom.begin(serial, onReceived);
}

// メインループ
void SprReceiverBLE::loop()
{
  // シリアルコマンド受信
  serialCom.loop();
  
  // 切断チェック
  disconnect_check();
}
