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
    int ret = stat(MNT_DIR, &st);
    if (ret == 0) return 1; // すでにマウントされている

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
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        Serial.print("JSON parse error: ");
        Serial.println(err.c_str());
        return false;
    }

    // 値を取得
    JsonObject params = doc["servoParams"];
    if (params.isNull()) {
        Serial.println("servoParams not found.");
        return false;
    }
    Kx   = params["Kx"]   | 120.0f;
    Ky   = params["Ky"]   | 60.0f;
    Vmax = params["Vmax"] | 2.0f;

    Serial.println("Loaded parameters:");
    Serial.println(Kx);
    Serial.println(Ky);
    Serial.println(Vmax);

    return true;
}

bool SdCard::save()
{
    // JSON ドキュメント作成
    StaticJsonDocument<256> doc;
    JsonObject params = doc.createNestedObject("servoParams");
    params["Kx"] = Kx;
    params["Ky"] = Ky;
    params["Vmax"] = Vmax;

    // SD カードに書き込み
    SD.remove("/config.json");
    File file = SD.open("/config.json", FILE_WRITE);
    if (!file) return false;
    serializeJson(doc, file);
    file.close();

    return true; 
}