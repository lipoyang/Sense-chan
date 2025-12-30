#include <LovyanGFX.h>
#include <Display.h>

// ピン割り当て
#define TFT_MISO      -1 // 接続しない
#define TFT_MOSI      11
#define TFT_SCLK      13
#define TFT_CS        10
#define TFT_DC        8
#define TFT_RST       7
#define TFT_BL        9

// 画面デバイスの定義
class MyLGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789  _panel_instance;
//lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
//lgfx::Light_PWM     _light_instance;

public:
  MyLGFX(void)
  {
    { // SPIバスの設定
      auto cfg = _bus_instance.config();

      cfg.spi_port = 4;          // SPRESENSEのSPIポート番号
      cfg.spi_mode = 3;          // SPI通信モード (0 ~ 3)
      cfg.freq_write = 40000000; // 送信時のSPIクロック
      cfg.freq_read = 20000000;  // 受信時のSPIクロック

      cfg.pin_sclk = TFT_SCLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TFT_MISO;
      cfg.pin_dc   = TFT_DC;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    { // 表示パネルの設定
      auto cfg = _panel_instance.config();

      cfg.pin_cs   = TFT_CS;
      cfg.pin_rst  = TFT_RST;
      cfg.pin_busy = -1; // 接続しない

      cfg.panel_width  = 240;   // 幅
      cfg.panel_height = 320;   // 高さ
      cfg.offset_x = 0;         // X方向オフセット
      cfg.offset_y = 0;         // Y方向オフセット

      cfg.invert = true;        // 明暗が反転する場合 true

      _panel_instance.config(cfg);
    }
#if 0
    { // バックライトの設定
      auto cfg = _light_instance.config();

      cfg.pin_bl = TFT_BL;
      cfg.invert = false;       // 輝度が反転する場合 true
      cfg.freq   = 44100;       // バックライトのPWM周波数
      cfg.pwm_channel = 0;      // 使用するPWMのチャンネル番号 (注意!)

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
#endif
    setPanel(&_panel_instance);
  }
};

SET_DISPLAY_CLASS(MyLGFX);
