// ファイルシステム版u8g2フォント (SPRESENSE用)

#include <stdlib.h>
#include <algorithm>

#include "FS_U8g2font.h"

#define _DEBUG_
#ifdef _DEBUG_
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

namespace lgfx
{
 inline namespace v1
 {
  // フォントのロード
  // path : フォントファイルのパス
  // 戻り値 : 成否
  bool FS_U8g2font::loadFont(const char* path)
  {
    _file = fopen(path, "rb");
    if (_file == nullptr)
    {
      DEBUG_PRINT("Can not open %s\n", path);
      return false;
    }

    // ヘッダをロード
    {
      const size_t size = NEW_HEADER_SIZE;
      size_t n = fread(_header, 1, size, _file);
      if (n < size) {
        DEBUG_PRINT("fread error (%zu != %zu) line %d\n", n, size, __LINE__);
        fclose(_file);
        return false;
      }

      // ヘッダサイズ (プリロード文字ブロック情報含む)
      _header_size = _header[23] << 8 | _header[24];
      // LUTサイズ(バイト数)
      _lut_size    = _header[25] << 8 | _header[26];
      // プリロード文字ブロックの数
      _preload_cnt = _header[27];

      // DEBUG_PRINT("_header_size = %d\n", _header_size);
      // DEBUG_PRINT("_lut_size    = %d\n", _lut_size);
      // DEBUG_PRINT("_preload_cnt = %d\n", _preload_cnt);
    }

    // プリロード文字ブロック情報をロード
    {
      const size_t size = _preload_cnt * 12;
      uint8_t* buffer = (uint8_t*)malloc(size);
      _preload_info = (PreloadInfo*)malloc(sizeof(PreloadInfo) * _preload_cnt);
      if (buffer == nullptr || _preload_info == nullptr){
        DEBUG_PRINT("malloc error line %d\n", __LINE__);
        if (buffer) free(buffer);
        unloadFont();
        return false;
      }
      size_t n = fread(buffer, 1, size, _file);
      if (n < size) {
        DEBUG_PRINT("fread error (%zu != %zu) line %d\n", n, size, __LINE__);
        if (buffer) free(buffer);
        unloadFont();
        return false;
      }
      for(int i = 0; i < _preload_cnt; ++i)
      {
        _preload_info[i].start_code = (buffer[i * 12 + 0] << 8) | buffer[i * 12 + 1];
        _preload_info[i].end_code   = (buffer[i * 12 + 2] << 8) | buffer[i * 12 + 3];
        _preload_info[i].start_addr = 
          (buffer[i * 12 +  4] << 24) | (buffer[i * 12 +  5] << 16) |
          (buffer[i * 12 +  6] <<  8) |  buffer[i * 12 +  7];
        _preload_info[i].block_size = 
          (buffer[i * 12 +  8] << 24) | (buffer[i * 12 +  9] << 16) |
          (buffer[i * 12 + 10] <<  8) |  buffer[i * 12 + 11];
      }
      free(buffer);
    }

    // ASCII文字および U+00FFまでの文字をロード
    {
      const size_t size = start_pos_unicode();
      _ascii = (uint8_t*)malloc(size);
      if (_ascii == nullptr) {
        DEBUG_PRINT("malloc error line %d\n", __LINE__);
        unloadFont();
        return false;
      }
      size_t n = fread(_ascii, 1, size, _file);
      if (n < size) {
        DEBUG_PRINT("fread error (%zu != %zu) line %d\n", n, size, __LINE__);
        unloadFont();
        return false;
      }
    }

    // LUTをロード
    {
      const size_t size = _lut_size;
      _lut = (uint8_t*)malloc(size);
      if (_lut == nullptr) {
        DEBUG_PRINT("malloc error line %d\n", __LINE__);
        unloadFont();
        return false;
      }
      size_t n = fread(_lut, 1, size, _file);
      if (n < size) {
        DEBUG_PRINT("fread error (%zu != %zu) line %d\n", n, size, __LINE__);
        unloadFont();
        return false;
      }
    }

    // プリロード文字ブロックをロード
    {
      for (int i = 0; i < _preload_cnt; ++i)
      {
        const uint32_t size = _preload_info[i].block_size;
        const uint32_t addr = _preload_info[i].start_addr;
        _preload[i] = (uint8_t*)malloc(size);
        if (_preload[i] == nullptr) {
          DEBUG_PRINT("malloc error line %d\n", __LINE__);
          unloadFont();
          return false;
        }
        if (seekRead(_preload[i], addr, size) == false) {
          unloadFont();
          return false;
        }
      }
    }
    return true;
  }

  // フォントのアンロード
  // 戻り値 : 成否
  bool FS_U8g2font::unloadFont(void)
  {
    unloadGlyphCache();
    unloadGlyph();
    for (int i = 0; i < _preload_cnt; i++) {
      if (_preload[i]) {
        free(_preload[i]);
        _preload[i] = nullptr;
      }
    }
    if (_preload_info) free(_preload_info);
    if (_ascii)        free(_ascii);
    if (_lut)          free(_lut);
    int ret = fclose(_file);
    return (ret == 0);
  }
  
  // グリフがロードされているかどうか
  bool FS_U8g2font::isGlyphLoaded(uint16_t uniCode) const
  {
    // ASCII文字等
    if (uniCode < 0x100) return true;

    // プリロード文字ブロックに含まれる文字
    for (int i = 0; i < _preload_cnt; ++i)
    {
      if (uniCode >= _preload_info[i].start_code && uniCode <= _preload_info[i].end_code) {
        return true;
      }
    }
    // すでにロードされている文字
    for (const GlyphEntry& v : _glyph_data) {
      if (uniCode == v.uniCode) return true;
    }

    return false; // 未ロード
  }

  // 動的使用グリフのブロック先頭アドレスをLUTから検索
  void FS_U8g2font::getGlyphBlockAddr(uint16_t uniCode, uint32_t* addr, uint32_t* size) const
  {
    uint_fast16_t e;
    *addr = _header_size + start_pos_unicode(); // グリフデータ先頭アドレス(ファイル上の)
    uint8_t* unicode_lut = _lut;  // LUTの先頭アドレス
    do
    {
      // DEBUG_PRINT("FS code = %04X : %04X, %04X @ %08X\n", uniCode,
      //   (unicode_lut[0] << 8) + unicode_lut[1],
      //   (unicode_lut[2] << 8) + unicode_lut[3],
      //   *addr);

      *addr += (unicode_lut[0] << 8) + unicode_lut[1];
      e = (unicode_lut[2] << 8) + unicode_lut[3];
      unicode_lut += 4;
    } while (e < uniCode);
    // DEBUG_PRINT("FS code = %04X : %04X, %04X @ %08X\n", uniCode,
    //   (unicode_lut[0] << 8) + unicode_lut[1],
    //   (unicode_lut[2] << 8) + unicode_lut[3],
    //   *addr);

    *size = (unicode_lut[0] << 8) + unicode_lut[1];
  }

  // ファイルから動的使用グリフのブロックを読み出す
  bool FS_U8g2font::seekRead(uint8_t* block, uint32_t addr, uint32_t size)
  {
    int ret = fseek(_file, addr, SEEK_SET);
    if (ret != 0) {
      DEBUG_PRINT("fseek failed (ret=%d) line %d\n", ret, __LINE__);
      return false;
    }
    size_t n = fread(block, 1, size, _file);
    if (n < size) {
      DEBUG_PRINT("fread error (%zu != %lu) line %d\n", n, size, __LINE__);
      return false;
    }
    return true;
  }

  // 動的使用文字のロード
  // uniCode : 文字コード
  bool FS_U8g2font::loadGlyph(uint16_t uniCode)
  {
    // ロード済みのグリフはスキップ
    if (isGlyphLoaded(uniCode)) return true;
    
    // LUTからブロック先頭アドレスを検索
    uint32_t addr = 0, size = 0;
    getGlyphBlockAddr(uniCode, &addr, &size);
    // DEBUG_PRINT("code = %04X : LUT addr = %08X, size = %08X\n", uniCode, addr, size);

    // ファイルからブロックごとデータを読み出し
    uint8_t* block = (uint8_t*)malloc(size);
    if (block == nullptr) {
      DEBUG_PRINT("malloc error line %d\n", __LINE__);
      return false;
    }
    if (seekRead(block, addr, size) == false) {
      free(block);
      return false;
    }

    // ブロック内からグリフデータを検索
    uint8_t* font = block;
    uint_fast16_t e;
    while(true)
    {
      size_t offset = (size_t)(font - block);
      if (offset + 3 > size) break;

      e = (uint_fast16_t)((font[0] << 8) | font[1]);
      uint8_t entry_size = font[2];
      if (entry_size == 0) break;
      if (offset + entry_size > size) break;

      if (e == uniCode) {
        // グリフデータを追加
        GlyphEntry entry{};
        entry.uniCode = uniCode;
        entry.glyph = (uint8_t*)malloc(entry_size);
        if (entry.glyph == nullptr) {
          DEBUG_PRINT("malloc error line %d\n", __LINE__);
          free(block);
          return false;
        }
        memcpy(entry.glyph, font + 3, entry_size - 3);
        _glyph_data.push_back(entry);
        return true;
      }
      font += entry_size;
    }
    free(block);

    // グリフが見つからなかった場合
    return false; 
  }

  // 動的使用文字のロード
  // str : 文字列
  bool FS_U8g2font::loadGlyph(const char* str)
  {
    _decoderState = utf8_decode_state_t::utf8_state0;
    if (str && str[0]) {
      auto tmp = str;
      do {
        uint16_t uniCode = *tmp;
        do {
          uniCode = decodeUTF8(*tmp);
        } while (uniCode < 0x20 && *++tmp);
        if (uniCode < 0x20) break;
        {
          // DEBUG_PRINT("uniCode = %04X\n", uniCode);
          if (!loadGlyph(uniCode)) return false;
        }
      } while (*++tmp);
    }
    return true;
  }

  // 動的使用文字のアンロード
  bool FS_U8g2font::unloadGlyph()
  {
    for (const GlyphEntry& v : _glyph_data) {
      if(v.glyph){
        free(v.glyph);
      }
    }
    _glyph_data.clear();
    return true;
  }

  // 一時使用文字のロード
  // uniCode : 文字コード
  bool FS_U8g2font::loadGlyphCache(uint16_t uniCode)
  {
    // ロード済みのグリフはスキップ
    if (isGlyphLoaded(uniCode)) return true;

    // すでにキャッシュされている文字
    for (const GlyphEntry& v : _glyph_cache) {
      if (uniCode == v.uniCode) return true;
    }

    // LUTからブロック先頭アドレスを検索
    uint32_t addr = 0, size = 0;
    getGlyphBlockAddr(uniCode, &addr, &size);

    // ファイルからブロックごとデータを読み出し
    uint8_t* block = (uint8_t*)malloc(size);
    if (block == nullptr) {
      DEBUG_PRINT("malloc error line %d\n", __LINE__);
      return false;
    }
    if (seekRead(block, addr, size) == false) {
      free(block);
      return false;
    }

    // ブロック内からグリフデータを検索
    uint8_t* font = block;
    uint_fast16_t e;
    while(true)
    {
      size_t offset = (size_t)(font - block);
      if (offset + 3 > size) break;

      e = (uint_fast16_t)((font[0] << 8) | font[1]);
      uint8_t entry_size = font[2];
      if (entry_size == 0) break;
      if (offset + entry_size > size) break;

      if (e == uniCode) {
        // グリフデータを追加
        GlyphEntry entry{};
        entry.uniCode = uniCode;
        entry.glyph = (uint8_t*)malloc(entry_size);
        if (entry.glyph == nullptr) {
          DEBUG_PRINT("malloc error line %d\n", __LINE__);
          free(block);
          return false;
        }
        memcpy(entry.glyph, font + 3, entry_size - 3);
        _glyph_cache.push_back(entry);

        // キャッシュサイズを超えた場合は最も古いグリフを削除
        if (_glyph_cache.size() > (size_t)_glyph_cache_size) {
          GlyphEntry old_entry = _glyph_cache.front();
          if (old_entry.glyph) {
            free(old_entry.glyph);
          }
          _glyph_cache.pop_front();
        }
        return true;
      }
      font += entry_size;
    }
    free(block);

    // グリフが見つからなかった場合
    return false;
  }

  // 一時使用文字のロード
  // str : 文字列
  bool FS_U8g2font::loadGlyphCache(const char* str)
  {
    _decoderState = utf8_decode_state_t::utf8_state0;
    if (str && str[0]) {
      auto tmp = str;
      do {
        uint16_t uniCode = *tmp;
        do {
          uniCode = decodeUTF8(*tmp);
        } while (uniCode < 0x20 && *++tmp);
        if (uniCode < 0x20) break;
        {
          if(!loadGlyphCache(uniCode)) return false;
        }
      } while (*++tmp);
    }
    return true;
  }

  // 一時使用文字のアンロード
  bool FS_U8g2font::unloadGlyphCache()
  {
    for (const GlyphEntry& v : _glyph_cache) {
      if (v.glyph) {
        free(v.glyph);
      }
    }
    _glyph_cache.clear();
    return true;
  }

  // UTF-8のデコード
  // c : UTF-8の文字列を1バイトずつ与える
  // 戻り値 : UTF-16のコード または 0
  uint16_t FS_U8g2font::decodeUTF8(uint8_t c)
  {
    // 7 bit Unicode Code Point
    if (!(c & 0x80)) {
      _decoderState = utf8_decode_state_t::utf8_state0;
      return c;
    }

    if (_decoderState == utf8_decode_state_t::utf8_state0)
    {
      // 11 bit Unicode Code Point
      if ((c & 0xE0) == 0xC0)
      {
        _unicode_buffer = ((c & 0x1F) << 6);
        _decoderState = utf8_decode_state_t::utf8_state1;
        return 0;
      }

      // 16 bit Unicode Code Point
      if ((c & 0xF0) == 0xE0)
      {
        _unicode_buffer = ((c & 0x0F) << 12);
        _decoderState = utf8_decode_state_t::utf8_state2;
        return 0;
      }
      // 21 bit Unicode  Code Point not supported so fall-back to extended ASCII
      //if ((c & 0xF8) == 0xF0) return (uint16_t)c;
    }
    else
    {
      if (_decoderState == utf8_decode_state_t::utf8_state2)
      {
        _unicode_buffer |= ((c & 0x3F) << 6);
        _decoderState = utf8_decode_state_t::utf8_state1;
        return 0;
      }
      _unicode_buffer |= (c & 0x3F);
      _decoderState = utf8_decode_state_t::utf8_state0;
      return _unicode_buffer;
    }

    _decoderState = utf8_decode_state_t::utf8_state0;

    return c; // fall-back to extended ASCII
  }

  struct fs_u8g2_font_decode_t
  {
    fs_u8g2_font_decode_t(const uint8_t* ptr) : decode_ptr(ptr), decode_bit_pos(0) {}

    const uint8_t* decode_ptr;      /* pointer to the compressed data */
    uint8_t decode_bit_pos;     /* bitpos inside a byte of the compressed data */

    uint_fast8_t get_unsigned_bits(uint_fast8_t cnt)
    {
      uint_fast8_t bit_pos = this->decode_bit_pos;
      uint_fast8_t val = *(this->decode_ptr) >> bit_pos;

      auto bit_pos_plus_cnt = bit_pos + cnt;
      if ( bit_pos_plus_cnt >= 8 )
      {
        bit_pos_plus_cnt -= 8;
        val |= *(++this->decode_ptr) << (8-bit_pos);
      }
      this->decode_bit_pos = bit_pos_plus_cnt;
      return val & ((1U << cnt) - 1);
    }

    int_fast8_t get_signed_bits(uint_fast8_t cnt)
    {
      return (int_fast8_t)get_unsigned_bits(cnt) - (1 << (cnt-1));
    }
  };

  const uint8_t* FS_U8g2font::getGlyph(uint16_t encoding) const
  {
    const uint8_t *font;

    // ASCII文字等
    if ( encoding <= 0x00FF )
    {
      font = _ascii;
      if ( encoding >= 'a' )      { font += this->start_pos_lower_a(); }
      else if ( encoding >= 'A' ) { font += this->start_pos_upper_A(); }

      for ( ; font[1]; font += font[1])
      {
        if ( font[0] == encoding ) { return font + 2; }  /* skip encoding and glyph size */
      }
      return nullptr;
    }
    else
    {
      // プリロード文字ブロック
      for (int i = 0; i < _preload_cnt; ++i)
      {
        if (encoding >= _preload_info[i].start_code && encoding <= _preload_info[i].end_code) {
          font = _preload[i];
          for (; 0 != (font[0] << 8 | font[1]); font += font[2])
          {
            if ((font[0] << 8 | font[1]) == encoding) { return font + 3; }  /* skip encoding and glyph size */
          }
          return nullptr;
        }
      }
      // 動的使用グリフ
      for (const GlyphEntry& v : _glyph_data) {
        if (v.uniCode == encoding) {
          return v.glyph;
        }
      }
      // 一時使用グリフ
      for (const GlyphEntry& v : _glyph_cache) {
        if (v.uniCode == encoding) {
          return v.glyph;
        }
      }
    }
    return nullptr;
  }

  void FS_U8g2font::getDefaultMetric(lgfx::FontMetrics *metrics) const
  {
    metrics->height    = max_char_height();
    metrics->y_advance = metrics->height;
    metrics->baseline  = metrics->height + y_offset();
    metrics->y_offset  = -metrics->baseline;
    metrics->x_offset  = 0;
  }

  bool FS_U8g2font::updateFontMetric(lgfx::FontMetrics *metrics, uint16_t uniCode) const
  {
    //printf("updateFontMetric %0X\n",uniCode );
    fs_u8g2_font_decode_t decode(getGlyph(uniCode));
    if ( decode.decode_ptr )
    {
      metrics->width     = decode.get_unsigned_bits(this->bits_per_char_width());
                          decode.get_unsigned_bits(this->bits_per_char_height());
      metrics->x_offset  = decode.get_signed_bits  (this->bits_per_char_x());
                          decode.get_signed_bits  (this->bits_per_char_y());
      metrics->x_advance = decode.get_signed_bits  (this->bits_per_delta_x());
      return true;
    }
    metrics->width = metrics->x_advance = this->max_char_width();
    metrics->x_offset = 0;
    return false;
  }

  size_t FS_U8g2font::drawChar(LGFXBase* gfx, int32_t x, int32_t y, uint16_t uniCode, const TextStyle* style, FontMetrics* metrics, int32_t& filled_x) const
  {
    int32_t sy = 65536 * style->size_y;
    y += (metrics->y_offset * sy) >> 16;
    fs_u8g2_font_decode_t decode(getGlyph(uniCode));
    if ( decode.decode_ptr == nullptr ) return drawCharDummy(gfx, x, y, this->max_char_width(), metrics->height, style, filled_x);

    uint32_t w = decode.get_unsigned_bits(bits_per_char_width());
    uint32_t h = decode.get_unsigned_bits(bits_per_char_height());

    int32_t sx = 65536 * style->size_x;

    int32_t xoffset = (decode.get_signed_bits(bits_per_char_x()) * sx) >> 16;

    int32_t yoffset = -(int32_t)(decode.get_signed_bits(bits_per_char_y()) + h + metrics->y_offset);

    int32_t xAdvance = (decode.get_signed_bits(bits_per_delta_x()) * sx) >> 16;

    uint32_t colortbl[2] = {gfx->getColorConverter()->convert(style->back_rgb888), gfx->getColorConverter()->convert(style->fore_rgb888)};
    bool fillbg = (style->back_rgb888 != style->fore_rgb888);
    int32_t left  = 0;
    int32_t right = 0;
    if (fillbg) {
      left  = std::max<int>(filled_x, x + (xoffset < 0 ? xoffset : 0));
      right = x + std::max<int>(((w * sx) >> 16) + xoffset, xAdvance);
      filled_x = right;
      gfx->setRawColor(colortbl[0]);
    }
    x += xoffset;
    gfx->startWrite();

    if (left < right)
    {
      if (yoffset > 0) {
        gfx->writeFillRect(left, y, right - left, (yoffset * sy) >> 16);
      }
      int32_t y0 = ((yoffset + h)   * sy) >> 16;
      int32_t y1 = (metrics->height * sy) >> 16;
      if (y0 < y1) {
        gfx->writeFillRect(left, y + y0, right - left, y1 - y0);
      }
    }

    if ( w > 0 )
    {
      if (left < right)
      {
        int32_t y0  = (  yoffset      * sy) >> 16;
        int32_t len = (((yoffset + h) * sy) >> 16) - y0;
        if (left < x)
        {
          gfx->writeFillRect(left, y + y0, x - left, len);
        }
        int32_t xwsx = x + ((w * sx) >> 16);
        if (xwsx < right)
        {
          gfx->writeFillRect(xwsx, y + y0, right - xwsx, len);
        }
      }
      left -= x;
      uint32_t ab[2];
      uint32_t lx = 0;
      uint32_t ly = 0;
      int32_t y0 = ((yoffset    ) * sy) >> 16;
      int32_t y1 = ((yoffset + 1) * sy) >> 16;
      do
      {
        ab[0] = decode.get_unsigned_bits(bits_per_0());
        ab[1] = decode.get_unsigned_bits(bits_per_1());
        bool i = 0;
        do
        {
          uint32_t length = ab[i];
          while (length)
          {
            uint32_t len = (length > w - lx) ? w - lx : length;
            length -= len;
            if (i || fillbg)
            {
              int32_t x0 = (lx * sx) >> 16;
              if (!i && x0 < left) x0 = left;
              int32_t x1 = ((lx + len) * sx) >> 16;
              if (x0 < x1)
              {
                gfx->setRawColor(colortbl[i]);
                gfx->writeFillRect( x + x0
                                  , y + y0
                                  , x1 - x0
                                  , y1 - y0);
              }
            }
            lx += len;
            if (lx == w)
            {
              lx = 0;
              ++ly;
              y0 = y1;
              y1 = ((ly + yoffset + 1) * sy) >> 16;
            }
          }
          i = !i;
        } while (i || decode.get_unsigned_bits(1) != 0 );
      } while (ly < h);
    }
    gfx->endWrite();
    return xAdvance;
  }
 }
}