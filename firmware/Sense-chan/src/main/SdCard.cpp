#include <Arduino.h>
#include <SDHCI.h>
#include <File.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <cxd56_gpio.h>
#include <cxd56_sdhci.h>

#include <ArduinoJson.h>

#include "SdCard.h"

SDClass SD;     // SDカード
File myFile;    // ファイル

// SDカードのリセット処理
// 戻り値: 結果 (0:未挿入, 正:リセット成功, 負:リセット失敗)
int sdcard_reset()
{
    // Sense-chan基板の設計ミスによる不具合回避のため。
    // SPRESENSEをH/WリセットするとSDカードが認識されなくなることがあるため
    // SDカードのS/Wリセット処理を行う。

    // CDピンをチェック (挿入時 LOW (false) )
    bool cd = cxd56_gpio_read(PIN_SDIO_CD);
    if(cd == true) return 0; // SDカード未挿入

    const char *DEV_FILE = "/dev/mmcsd0"; // SDカードのデバイスファイル
    const char *MNT_DIR  = "/mnt/sd0";    // SDカードのマウントポイント

    // /mnt/sd0 が正常にマウントされていれば何もしない
    struct stat st;
    int ret;
    for (int retry = 0; retry < 10; retry++) {
        ret = stat(MNT_DIR, &st);
        if (ret == 0){
            return true; // すでにマウントされている
        }
        usleep(100 * 1000); // 100 msec
        //Serial.print("R.");
    }

    // SDIOを初期化し、OSのデバイスドライバに知らせる
    struct sdio_dev_s *sdio = cxd56_sdhci_initialize(0);
    cxd56_sdio_resetstatus(sdio);
    cxd56_sdhci_mediachange(sdio);

    if (stat(DEV_FILE, &st) != 0) return -1; // デバイスファイルが無い
    if (stat(MNT_DIR,  &st) == 0) return  2; // すでにマウントされている

    // マウントする
    ret = mount(DEV_FILE, MNT_DIR, "vfat", 0, NULL);
    if (ret < 0) {
        perror("mount failed");
        return -2; // マウント失敗
    }else{
        return 3; // マウント成功
    }
}

bool SdCard::begin()
{
    int ret = sdcard_reset();
    if(ret < 0) {
        Serial.println("SD card reset failed");
        return false;
    } else if (ret == 0) {
        Serial.println("SD card not inserted");
        return false;
    }
    if(!SD.begin()) {
        Serial.println("SD card begin failed");
        return false;
    }
    return true;
}

bool SdCard::load()
{
    // JSON ファイルを開く
    File file = SD.open("/config.json");
    if (!file) {
        Serial.println("Failed to open config.json");
        return false;
    }

    // JSON を読み込む
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        Serial.print("JSON parse error: ");
        Serial.println(err.c_str());
        return false;
    }

    // 値を取得
    {
        JsonObject params = doc["servoParams"];
        if (params.isNull()) {
            Serial.println("servoParams not found.");
            return false;
        }
        Kx   = params["Kx"]   | 60.0f;
        Ky   = params["Ky"]   | 60.0f;
        Vmax = params["Vmax"] | 2.0f;
        Vfull = params["Vfull"] | 12.0f;
        Kp   = params["Kp"]   | 0.25f;
        Ki   = params["Ki"]   | 0.0f;
        Kd   = params["Kd"]   | 0.25f;
    }
    {
        JsonObject params = doc["softSerialPorts"];
        if (params.isNull()) {
            Serial.println("softSerialPorts not found.");
            return false;
        }
        RX   = params["RX"]   | 25;
        TX   = params["TX"]   | 26;
        Baud  = params["Baud"] | 115200;
    }
    {
        JsonObject params = doc["lcd"];
        if (params.isNull()) {
            Serial.println("lcd not found.");
            return false;
        }
        PWM   = params["PWM"] | 50.0f;
    }

    Serial.println("Loaded parameters:");
    Serial.println(Kx);
    Serial.println(Ky);
    Serial.println(Vmax);
    Serial.println(Vfull);
    Serial.println(Kp);
    Serial.println(Ki);
    Serial.println(Kd);
    Serial.println(RX);
    Serial.println(TX);
    Serial.println(Baud);
    Serial.println(PWM);

    return true;
}

bool SdCard::save()
{
    // JSON ドキュメント作成
    StaticJsonDocument<512> doc;
    {
        JsonObject params = doc.createNestedObject("servoParams");
        params["Kx"] = Kx;
        params["Ky"] = Ky;
        params["Vmax"] = Vmax;
        params["Vfull"] = Vfull;
        params["Kp"] = Kp;
        params["Ki"] = Ki;
        params["Kd"] = Kd;
    }
    {
        JsonObject params = doc.createNestedObject("softSerialPorts");
        params["RX"] = RX;
        params["TX"] = TX;
        params["Baud"] = Baud;
    }
    {
        JsonObject params = doc.createNestedObject("lcd");
        params["PWM"] = PWM;
    }

    // SD カードに書き込み
    SD.remove("/config.json");
    File file = SD.open("/config.json", FILE_WRITE);
    if (!file) return false;
    serializeJson(doc, file);
    file.close();

    return true; 
}