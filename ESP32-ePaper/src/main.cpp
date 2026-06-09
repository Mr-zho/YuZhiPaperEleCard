#include "myEPaper.h"
#include <SD.h>
#include "DataStorage.h"
#include <vector> 
#include <WiFi.h>
#include "BleReceiver.h"

BleReceiver *bleReceiver = nullptr;

EPaper ePaper(EPD_10in85_WIDTH * 2, EPD_10in85_HEIGHT);
SDStorage sd;


#define GLYPH_W   240
#define GLYPH_H   240
#define GLYPH_BPP 1
#define GLYPH_BYTES (GLYPH_W * GLYPH_H / 8)  // 32
uint8_t glyph_buf[GLYPH_BYTES];
String font_path = "/FangSongTi_240x240.bin";
std::vector<std::vector<uint8_t>> fontVector;
// std::vector<std::array<uint8_t, 7200>> fontVector;

fs::File font_fp;
// 自带颜色反转功能
bool tf_get_glyph_gb2312(uint16_t gb, uint8_t *out)
{
    if (!font_fp) 
    {
        Serial.println("Font file not open");
        return false;
    }

    uint8_t hi = gb >> 8;
    uint8_t lo = gb & 0xFF;

    if (hi < 0xA1 || hi > 0xF7 || lo < 0xA1 || lo > 0xFE)
    {
        Serial.printf("Invalid GB2312 code: 0x%X\n", gb);
        return false;
    }  

    uint32_t index =
        (hi - 0xA1) * 94 +
        (lo - 0xA1);

    uint32_t offset = index * GLYPH_BYTES;

    if (!font_fp.seek(offset))
    {
        Serial.printf("Failed to seek to offset: %u\n", offset);
        return false;
    }

    if (font_fp.read(out, GLYPH_BYTES) != GLYPH_BYTES)
    {
        Serial.println("Failed to read glyph data");
        return false;
    }

    return true;
}


bool read_be32(File &f, uint32_t *out)
{
    uint8_t b[4];
    if (f.read(b, 4) != 4) return false;
    *out = (uint32_t)b[0] << 24 |
           (uint32_t)b[1] << 16 |
           (uint32_t)b[2] << 8  |
           (uint32_t)b[3];
    return true;
}

bool read_be16(File &f, uint16_t *out)
{
    uint8_t b[2];
    if (f.read(b, 2) != 2) return false;
    *out = (uint16_t)b[0] << 8 |
           (uint16_t)b[1];
    return true;
}

// UTF-8 → Unicode
uint32_t utf8ToUnicode(const char *s, uint8_t *bytesUsed) {
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
bool unicode_to_gb2312(uint32_t unicode, uint16_t *gb)
{
    static File f;
    static bool opened = false;

    if (!opened) {
        f = SD.open("/unicode_gb2312.map", FILE_READ);
        if (!f) {
            Serial.println("Failed to open mapping file");
            return false;
        }
        opened = true;
    }

    f.seek(0);

    uint32_t u;
    uint16_t g;

    while (f.position() + 6 <= f.size()) {
        if (!read_be32(f, &u)) break;
        if (!read_be16(f, &g)) break;

        if (u == unicode) {
            *gb = g;
            return true;
        }
    }
    return false;
}




void setup() 
{
    Serial.begin(115200);

    bleReceiver = new BleReceiver();
    bleReceiver->setTextCallback([](const String& text)
    {
        Serial.print("Received via BLE: ");
        Serial.println(text);
        const char *p = text.c_str();
        if (p == nullptr || *p == '\0') return;
        std::vector<uint16_t> codeList;
        codeList.reserve(5);
        while (*p && codeList.size() < 5)
        {
            uint8_t len;
            uint32_t code = utf8ToUnicode(p, &len);

            // 如果 len 解析出来是 0（非法的 UTF-8 字节），强制让指针往后跳 1 字节，防止死循环
            if (len == 0) {
                Serial.println("Warning: Invalid UTF-8 byte detected, skipping 1 byte.");
                p++; 
                continue;
            }
            p += len;
            uint16_t gb;
            if(unicode_to_gb2312(code, &gb)) {
                codeList.push_back(gb);
            } else {
                Serial.printf("No GB2312 mapping for U+%04X\n", code);
            }
        }

        if (!codeList.empty())
        {
            fontVector.clear();
            Serial.print("Parsed gb2312 code points: ");
            for (size_t i = 0; i < codeList.size(); i++)
            {
                Serial.printf("0x%X ", codeList[i]);
                if(tf_get_glyph_gb2312(codeList[i], glyph_buf)) // “永”
                {
                    // 通过传入数组首地址和结束地址，让 vector 自动复制这 7200 字节
                    fontVector.push_back(std::vector<uint8_t>(glyph_buf, glyph_buf + GLYPH_BYTES));
                    Serial.println("glyph loaded");
                } else {
                    Serial.println("Failed to load glyph");
                }
            }
            if (!fontVector.empty()) {
                ePaper.Display(0, 0, fontVector, BLACK, WHITE, WHITE);
            }
            Serial.println("Update display with new font data");
        }
    });

    bleReceiver->begin("ePaper-BLE");
    
  
    Serial.print("Init SD...\n");
    if(sd.SD_Init()) Serial.println("SD is ok\n");
    else 
    {
        Serial.println("SD is failed\n");
        while(1);
    }

    // 打开文件（只读模式）
    font_fp = SD.open(font_path); 
    if (!font_fp) 
    {
        Serial.println("Failed to open font file");
        while(1);
    }

    ePaper.EPaper_Init();
    ePaper.EPaper_DisSleep();

    Serial.printf("steup completed\n");
    return ;
}


void loop() 
{
    delay(5000);
}
