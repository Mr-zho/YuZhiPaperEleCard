#ifndef BITMAP_FONT_H
#define BITMAP_FONT_H

#include <FS.h>
#include <functional>
#include "lvgl.h"

class BitmapFont 
{
public:
    BitmapFont() {};
    ~BitmapFont() = default;

private:
    uint GLYPH_W = 0;
    uint GLYPH_H = 0;    
    uint GLYPH_BPP = 1;
    uint GLYPH_BYTES = 0;  
    String font_path = "";
    String map_path = "";
    fs::File font_fp;
    fs::File fontMap_fp;
    std::function<bool(fs::File &fp, const String &path)> OpenFontFile = nullptr;

    // 读取大端32位整数
    bool read_be32(fs::File &f, uint32_t *out) {
        uint8_t b[4];
        if (f.read(b, 4) != 4) return false;
        *out = (uint32_t)b[0] << 24 |
               (uint32_t)b[1] << 16 |
               (uint32_t)b[2] << 8  |
               (uint32_t)b[3];
        return true;
    }

    // 读取大端16位整数
    bool read_be16(fs::File &f, uint16_t *out) {
        uint8_t b[2];
        if (f.read(b, 2) != 2) return false;
        *out = (uint16_t)b[0] << 8 |
               (uint16_t)b[1];
        return true;
    }

public:
    // UTF-8 → Unicode
    uint32_t UTF8ToUnicode(const char *s, uint8_t *bytesUsed) {
        uint8_t c = (uint8_t)s[0];
        if ((c & 0x80) == 0) {
            *bytesUsed = 1;
            return c;
        } else if ((c & 0xE0) == 0xC0) {
            *bytesUsed = 2;
            return ((c & 0x1F) << 6) |
                   (s[1] & 0x3F);
        } else if ((c & 0xF0) == 0xE0) {
            *bytesUsed = 3;
            return ((c & 0x0F) << 12) |
                   ((s[1] & 0x3F) << 6) |
                   (s[2] & 0x3F);
        } else if ((c & 0xF8) == 0xF0) {
            *bytesUsed = 4;
            return ((c & 0x07) << 18) |
                   ((s[1] & 0x3F) << 12) |
                   ((s[2] & 0x3F) << 6) |
                   (s[3] & 0x3F);
        }
        *bytesUsed = 0;
        return 0;
    }

    // Unicode → GB2312
    bool UnicodeToGB2312(uint32_t unicode, uint16_t *gb) {
        if (!fontMap_fp) {
            Serial.println("Failed to open mapping file");
            return false;
        }
        fontMap_fp.seek(0);

        uint32_t u;
        uint16_t g;

        while (fontMap_fp.position() + 6 <= fontMap_fp.size()) {
            if (!read_be32(fontMap_fp, &u)) break;
            if (!read_be16(fontMap_fp, &g)) break;
            if (u == unicode) {
                *gb = g;
                return true;
            }
        }
        return false;
    }

    // 获取 GB2312 字符的点阵数据，带颜色反转
    bool GetBitmapByGB2312(uint16_t gb, uint8_t *out) {
        if (!font_fp) return false;

        uint8_t hi = gb >> 8;
        uint8_t lo = gb & 0xFF;

        if (hi < 0xA1 || hi > 0xF7 || lo < 0xA1 || lo > 0xFE)
            return false;

        uint32_t index = (hi - 0xA1) * 94 + (lo - 0xA1);
        uint32_t offset = index * GLYPH_BYTES;

        if (!font_fp.seek(offset))
            return false;

        if (font_fp.read(out, GLYPH_BYTES) != GLYPH_BYTES)
            return false;

        for (uint32_t i = 0; i < GLYPH_BYTES; i++)
            out[i] = ~out[i];  // 颜色反转

        return true;
    }

    // 设置自定义文件打开回调
    void SetOpenFontFileFunction(std::function<bool(fs::File &fp, const String &path)> callback) {
        OpenFontFile = callback;
    }

    // 初始化字体系统
    bool Begin(const String &font_path, const String &map_path, uint glyph_w, uint glyph_h) {
        this->font_path = font_path;
        this->map_path = map_path;
        this->GLYPH_W = glyph_w;
        this->GLYPH_H = glyph_h;
        this->GLYPH_BYTES = (GLYPH_W * GLYPH_H / 8);
        if (OpenFontFile) {
            if (!OpenFontFile(font_fp, font_path)) {
                Serial.println("Failed to open font file via callback");
                return false;
            }
            if (!OpenFontFile(fontMap_fp, map_path)) {
                Serial.println("Failed to open mapping file via callback");
                return false;
            }
        } else {
            Serial.println("Failed to open font file");
            return false;
        }
        return true;
    }

    // UTF-8 字符 → 点阵
    bool UTF8ToBitmap(const char *s, uint8_t *out, uint8_t *bytesUsed) {
        uint32_t unicode = UTF8ToUnicode(s, bytesUsed);
        uint16_t gb;
        if (!UnicodeToGB2312(unicode, &gb)) {
            Serial.println("Failed to convert Unicode to GB2312");
            Serial.print("Unicode: ");
            Serial.println(unicode, HEX);
            return false;
        }
        return GetBitmapByGB2312(gb, out);
    }

    /******************************************
     * LVGL 8.4.0 字体接口
     ******************************************/
    static bool lv_font_bitmap_get_glyph(const lv_font_t *font,
                                     lv_font_glyph_dsc_t *dsc_out,
                                     uint32_t unicode_letter,
                                     uint32_t unicode_next)
    {
        BitmapFont *bf = (BitmapFont *)font->user_data;
        if (!bf || !dsc_out) return false;

        uint16_t gb;
        if (!bf->UnicodeToGB2312(unicode_letter, &gb)) return false;

        static uint8_t buf[1024];  // 单个字节上限
        if (!bf->GetBitmapByGB2312(gb, buf)) return false;

        dsc_out->adv_w  = bf->GLYPH_W;
        dsc_out->box_h  = bf->GLYPH_H;
        dsc_out->box_w  = bf->GLYPH_W;
        dsc_out->ofs_x  = 0;
        dsc_out->ofs_y  = 0;
        dsc_out->bpp    = bf->GLYPH_BPP;

        // LVGL 8.x 不再有 glyph_bitmap，字形数据通过 get_glyph_bitmap 返回
        return true;
    }

    // 生成 LVGL 字体对象
    void InitLVGLFont(lv_font_t *lv_font) {
        lv_font->get_glyph_dsc = lv_font_bitmap_get_glyph;
        lv_font->get_glyph_bitmap = nullptr; // 位图直接在 get_glyph_dsc 提供
        lv_font->line_height = GLYPH_H;
        lv_font->base_line = 0;
        lv_font->subpx = LV_FONT_SUBPX_NONE;
        lv_font->user_data = this;
        lv_font->dsc = nullptr;
    }

};

// 字体映射文件接口, 用于自定义文件系统
class FontStream {
public:
    virtual ~FontStream() {}
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool seek(uint32_t pos) = 0;
    virtual int  read(uint8_t *buf, size_t len) = 0;
};

#endif // BITMAP_FONT_H