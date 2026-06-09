#ifndef MY_E_PAPER_H
#define MY_E_PAPER_H
#include "EPD_10in85.h"
// 绘图库
#include "GUI_Paint.h"
// 字体库
#include "fonts.h"
// 图像数据
#include "ImageData.h"
#include <cstdint>
#include <vector>
#include <SD.h>

class EPaper 
{
private:
    uint8_t font_scale = 1;             // 字体放大倍数，默认2倍
    File fontFile;
    uint16_t font_Height = 124 * 1;     
    uint16_t font_Width = 96 * 1;
    uint16_t width_, height_;
    uint16_t font_space = font_Width * font_scale;
    UBYTE *Image;
    // 定义统一的函数包装类型
    using SetPixelFuncWrapper = std::function<void(uint16_t, uint16_t, uint16_t)>;
    SetPixelFuncWrapper wrapper1 = std::bind(&EPaper::SetPixel_1, this, 
                                   std::placeholders::_1, 
                                   std::placeholders::_2, 
                                   std::placeholders::_3);
    SetPixelFuncWrapper wrapper2 = std::bind(&EPaper::SetPixel_2, this, 
                                    std::placeholders::_1, 
                                    std::placeholders::_2, 
                                    std::placeholders::_3);

    void sendImage_1(UBYTE* Image);
    void sendImage_2(UBYTE* Image);
    void DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString, cFONT* font,
                        UWORD Color_Foreground, UWORD Color_Background, 
                        SetPixelFuncWrapper& setPixel);
  
    void DrawFont(UWORD x, UWORD y, std::vector<std::vector<uint8_t>>& fontVector,
    unsigned char Color_Foreground, unsigned char Color_Background, SetPixelFuncWrapper& setPixel);

    uint8_t getPixelValue(const std::vector<uint8_t>& fontData, int x, int y);
    void drawFontWithScale1(UWORD x, UWORD y, std::vector<std::vector<uint8_t>>& fontVector,
                            unsigned char Color_Foreground, unsigned char Color_Background);
public:
    EPaper(uint16_t width, uint16_t height);
    ~EPaper() = default;

    UBYTE * GetImageBuffer() { return Image; }

    // 禁用拷贝构造和赋值运算
    EPaper(const EPaper&) = delete;
    EPaper& operator=(const EPaper&) = delete;

    void EPaper_Init();
    void EPaper_Close();
    void Display(UWORD Xstart, UWORD Ystart, std::vector<std::vector<uint8_t>>& font, 
                UWORD Color_Foreground, UWORD font_BackColor, UWORD screen_BackColor);
    void EPaper_Sleep();
    void EPaper_DisSleep();

    void SetPixel_1(UWORD Xpoint, UWORD Ypoint, UWORD Color);
    void SetPixel_2(UWORD Xpoint, UWORD Ypoint, UWORD Color);
};

#endif // SCREEN_SPLITTER_H
    
