#pragma once
#include <list>
#ifdef _PC_DEBUG
#pragma warning(disable:26495)
#include "PC_debug.h"
#else
#include <LovyanGFX.h>
#endif

// ファイルシステム版u8g2フォント (SPRESENSE用)
namespace lgfx
{
  inline namespace v1
  {
    // ファイルシステム版u8g2フォント
    struct FS_U8g2font : public lgfx::IFont
    {
      // UTF-8デコード用
      enum utf8_decode_state_t : uint8_t
      { utf8_state0 = 0
      , utf8_state1 = 1
      , utf8_state2 = 2
      };

      // プリロード文字ブロック情報
      struct PreloadInfo {
        uint16_t start_code;  // ブロックの開始コード
        uint16_t end_code;    // ブロックの終了コード
        uint32_t start_addr;  // ブロックの開始アドレス
        uint32_t block_size;  // ブロックのサイズ
      };

      // 動的/一時使用文字データ
      struct GlyphEntry
      {
        uint16_t uniCode; // 文字コード
        uint8_t* glyph;   // グリフデータ
      };

      // フォントのロードとアンロード
      bool loadFont(const char* path);
      bool unloadFont(void);

      // 動的使用文字のロードとアンロード
	    bool loadGlyph(uint16_t uniCode);
	    bool loadGlyph(const char* str);
      bool unloadGlyph();

      // 一時使用文字のロードとアンロード
	    void setGlyphCacheSize(uint16_t size) { _glyph_cache_size = (int)size; }
      bool loadGlyphCache(uint16_t uniCode);
      bool loadGlyphCache(const char* str);
      bool unloadGlyphCache();

      // とりあえず ft_u8g2 を返しておく
      font_type_t getType(void) const override { return ft_u8g2; }

      // 元のヘッダ23バイトはRAMにロードした値を使う
      uint8_t glyph_cnt (void) const { return _header[0]; }
      uint8_t bbx_mode  (void) const { return _header[1]; }
      uint8_t bits_per_0(void) const { return _header[2]; }
      uint8_t bits_per_1(void) const { return _header[3]; }
      uint8_t bits_per_char_width (void) const { return _header[4]; }
      uint8_t bits_per_char_height(void) const { return _header[5]; }
      uint8_t bits_per_char_x     (void) const { return _header[6]; }
      uint8_t bits_per_char_y     (void) const { return _header[7]; }
      uint8_t bits_per_delta_x    (void) const { return _header[8]; }
      int8_t max_char_width (void) const { return _header[9]; }
      int8_t max_char_height(void) const { return _header[10]; } /* overall height, NOT ascent. Instead ascent = max_char_height + y_offset */
      int8_t x_offset       (void) const { return _header[11]; }
      int8_t y_offset       (void) const { return _header[12]; }
      int8_t ascent_A    (void) const { return _header[13]; }
      int8_t descent_g   (void) const { return _header[14]; }  /* usually a negative value */
      int8_t ascent_para (void) const { return _header[15]; }
      int8_t descent_para(void) const { return _header[16]; }

      uint16_t start_pos_upper_A(void) const { return _header[17] << 8 | _header[18]; }
      uint16_t start_pos_lower_a(void) const { return _header[19] << 8 | _header[20]; }
      uint16_t start_pos_unicode(void) const { return _header[21] << 8 | _header[22]; }

      void getDefaultMetric(FontMetrics* metrics) const override;
      bool updateFontMetric(FontMetrics* metrics, uint16_t uniCode) const override;
      size_t drawChar(LGFXBase* gfx, int32_t x, int32_t y, uint16_t c, const TextStyle* style, FontMetrics* metrics, int32_t& filled_x) const override;

      const uint8_t* getGlyph(uint16_t encoding) const;

    private:
      static const int NEW_HEADER_SIZE = 23 + 5; // ヘッダサイズ (プリロード文字ブロック情報除く)
      static const int PRELOAD_MAX_CNT = 16;     // プリロード文字ブロックの最大数

//    const uint8_t* getGlyph(uint16_t encoding) const;
      
      FILE* _file; // フォントファイル
      uint8_t _header[NEW_HEADER_SIZE]; // フォントヘッダ

      // ヘッダ拡張部のデータ
      int _header_size;           // ヘッダサイズ (プリロード文字ブロック情報含む)
      int _lut_size;              // LUTサイズ
      int _preload_cnt;           // プリロード文字ブロックの数
      PreloadInfo* _preload_info; // プリロード文字ブロックの情報

      // 最初にロードしておくデータ
      uint8_t* _ascii;            // ASCIIグリフデータ
      uint8_t* _lut;              // LUTデータ
      uint8_t* _preload[PRELOAD_MAX_CNT]; // プリロード文字データ

      // 動的にロードするデータ
	    std::list<GlyphEntry> _glyph_data;  // 動的使用文字
      std::list<GlyphEntry> _glyph_cache; // 一時使用文字
      int _glyph_cache_size = 256; // 一時使用文字の最大キャッシュサイズ(文字数)

      // UTF-8デコード用
      uint16_t decodeUTF8(uint8_t c);
      utf8_decode_state_t _decoderState = utf8_state0;
      uint16_t _unicode_buffer = 0;

      // 動的ロード用
      bool isGlyphLoaded(uint16_t uniCode) const;
      void getGlyphBlockAddr(uint16_t uniCode, uint32_t *addr, uint32_t *size) const;
      bool seekRead(uint8_t* block, uint32_t addr, uint32_t size);
    };
  }
}
#pragma once
