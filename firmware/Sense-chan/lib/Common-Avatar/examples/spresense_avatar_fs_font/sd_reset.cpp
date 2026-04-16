#include <stdio.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <cxd56_gpio.h>
#include <cxd56_sdhci.h>

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
