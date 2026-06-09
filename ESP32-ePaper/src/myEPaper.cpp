#include "myEPaper.h"
#include <stdexcept>
#include <cassert>
#include <Arduino.h>


// 结构体：存储计算结果（包含是否成功的标志）
struct TextSpacing 
{
  bool success;    // true：计算成功；false：空间不足
  int spacing;     // 均匀间隔（像素），失败时为 -1
};

// 函数：单行显示 - 计算文字间间隔
// 参数：screenSize-屏幕尺寸（宽/高），fontSize-文字对应尺寸，textCount-文字数量，margin-边缘留白
TextSpacing calcSingleLineSpacing(int screenSize, int fontSize, int textCount, int margin = 0) 
{
  TextSpacing result = {false, -1};
  
  // 边界判断：文字数量至少为1，否则无需计算
  if (textCount <= 0) return result;
  // 单个文字无间隔，直接返回成功
  if (textCount == 1) 
  {  
    result.success = true;
    result.spacing = 0;
    return result;
  }
  
  // 计算可用空间和总间隔空间
  int availableSpace = screenSize - 2 * margin;
  int totalFontSpace = fontSize * textCount;
  int totalGapSpace = availableSpace - totalFontSpace;
  
  // 空间不足判断
  if (totalGapSpace < 0) return result;
  
  // 计算均匀间隔（整数取整，向下取整）
  result.spacing = totalGapSpace / (textCount - 1);
  result.success = true;
  return result;
}



// 构造函数实现
EPaper::EPaper(uint16_t width, uint16_t height)
    : width_(width), height_(height)
{

}
 


/*** 
 * 函数功能：设置像素
***/
void EPaper::SetPixel_1(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    UWORD X, Y;
    switch(Paint.Rotate) 
    {
    case 0:
        X = Xpoint;
        Y = Ypoint;  
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
    default:
        return;
    }
    
    switch(Paint.Mirror) 
    {
    case MIRROR_NONE:
        break;
    case MIRROR_HORIZONTAL:
        X = Paint.WidthMemory - X - 1;
        break;
    case MIRROR_VERTICAL:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case MIRROR_ORIGIN:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    if(X > Paint.WidthMemory)
    {
        return;
    }

    if(X > Paint.WidthMemory || Y > Paint.HeightMemory)
    {
        Debug("Exceeding display boundaries\r\n");
        return;
    }
    
    if(Paint.Scale == 2)
    {
        UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
        UBYTE Rdata = Paint.Image[Addr];
        if(Color == BLACK)
            Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
        else
            Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
    }else if(Paint.Scale == 4)
    {
        UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
        Color = Color % 4;//Guaranteed color scale is 4  --- 0~3
        UBYTE Rdata = Paint.Image[Addr];
        
        Rdata = Rdata & (~(0xC0 >> ((X % 4)*2)));
        Paint.Image[Addr] = Rdata | ((Color << 6) >> ((X % 4)*2));
    }else if(Paint.Scale == 6 || Paint.Scale == 7 || Paint.Scale == 16)
    {
		UDOUBLE Addr = X / 2  + Y * Paint.WidthByte;
		UBYTE Rdata = Paint.Image[Addr];
		Rdata = Rdata & (~(0xF0 >> ((X % 2)*4)));//Clear first, then set value
		Paint.Image[Addr] = Rdata | ((Color << 4) >> ((X % 2)*4));
		// printf("Add =  %d ,data = %d\r\n",Addr,Rdata);	
    }
}



/*** 
 * 函数功能：设置像素
***/
void EPaper::SetPixel_2(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    UWORD X, Y;
    switch(Paint.Rotate) 
    {
    case 0:
        X = Xpoint;
        Y = Ypoint;  
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
    default:
        return;
    }
    
    switch(Paint.Mirror) 
    {
    case MIRROR_NONE:
        break;
    case MIRROR_HORIZONTAL:
        X = Paint.WidthMemory - X - 1;
        break;
    case MIRROR_VERTICAL:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case MIRROR_ORIGIN:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    if(X >= Paint.WidthMemory)
    {
        X -= Paint.WidthMemory;
    }else
    {
        return;
    }

    if(X > Paint.WidthMemory || Y > Paint.HeightMemory)
    {
        Debug("Exceeding display boundaries\r\n");
        return;
    }
    
    if(Paint.Scale == 2)
    {
        UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
        UBYTE Rdata = Paint.Image[Addr];
        if(Color == BLACK)
            Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
        else
            Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
    }else if(Paint.Scale == 4)
    {
        UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
        Color = Color % 4;//Guaranteed color scale is 4  --- 0~3
        UBYTE Rdata = Paint.Image[Addr];
        
        Rdata = Rdata & (~(0xC0 >> ((X % 4)*2)));
        Paint.Image[Addr] = Rdata | ((Color << 6) >> ((X % 4)*2));
    }else if(Paint.Scale == 6 || Paint.Scale == 7 || Paint.Scale == 16)
    {
		UDOUBLE Addr = X / 2  + Y * Paint.WidthByte;
		UBYTE Rdata = Paint.Image[Addr];
		Rdata = Rdata & (~(0xF0 >> ((X % 2)*4)));//Clear first, then set value
		Paint.Image[Addr] = Rdata | ((Color << 4) >> ((X % 2)*4));
		// printf("Add =  %d ,data = %d\r\n",Addr,Rdata);	
    }
}



/***
 * 函数功能：绘制中文
 * 最近修改：2025.9.17
 */
void EPaper::DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString, cFONT* font,
                        UWORD Color_Foreground, UWORD Color_Background, 
                        SetPixelFuncWrapper& setPixel)
{
    const char* p_text = pString;
    int x = Xstart, y = Ystart;
    int i, j, Num;

    /* Send the string character by character on EPD */
    while (*p_text != 0) 
    {
        //ASCII < 126
        if((*p_text&0xff) <= 0x7F) 
        { 
            for(Num = 0; Num < font->size; Num++) 
            {
                if(*p_text== font->table[Num].index[0]) 
                {
                    const unsigned char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) 
                    {
                        for (i = 0; i < font->Width; i++) 
                        {
                            if (FONT_BACKGROUND == Color_Background) 
                            { 
                                if (*ptr & (0x80 >> (i % 8))) 
                                {
                                    setPixel(x + i, y + j, Color_Foreground);
                                }
                            } else 
                            {
                                if (*ptr & (0x80 >> (i % 8))) 
                                {
                                    setPixel(x + i, y + j, Color_Foreground);
                                } else 
                                {
                                    setPixel(x + i, y + j, Color_Background);
                                }
                            }
                            if (i % 8 == 7) 
                            {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) 
                        {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 1;
            /* Decrement the column position by 16 */
            x += font->ASCII_Width;
        } else 
        {   //Chinese
            for(Num = 0; Num < font->size; Num++) 
            {
                if ((((*p_text)&0xFF) == font->table[Num].index[0]) && \
                    (((*(p_text + 1))&0xFF) == font->table[Num].index[1]) && \
                    (((*(p_text + 2))&0xFF) == font->table[Num].index[2])) 
                    {
                    const unsigned char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) 
                    {
                        for (i = 0; i < font->Width; i++) 
                        {
                            if (FONT_BACKGROUND == Color_Background) 
                            { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) 
                                {
                                    setPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            } else 
                            {
                                if (*ptr & (0x80 >> (i % 8))) 
                                {
                                    setPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                } else 
                                {
                                    setPixel(x + i, y + j, Color_Background);
                                    // Paint_DrawPoint(x + i, y + j, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            }
                            if (i % 8 == 7) 
                            {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) 
                        {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 3;
            /* Decrement the column position by 16 */
            x += font->Width;
        }
    }
}



// 辅助函数：获取原字体中(x,y)位置的像素值（0为背景，1为前景）
uint8_t EPaper::getPixelValue(const std::vector<uint8_t>& fontData, int x, int y)
{
    // 计算(x,y)在点阵数据中的字节索引和位偏移
    int byteIndex = y * ((font_Width + 7) / 8) + (x / 8);
    int bitOffset = 7 - (x % 8);  // 点阵数据通常高位在前
    
    if (byteIndex >= fontData.size()) {
        return 0;  // 越界视为背景
    }
    // 返回0（背景）或1（前景）
    return (fontData[byteIndex] >> bitOffset) & 0x01;
}


// 1倍放大的原始逻辑（保持兼容性）
void EPaper::drawFontWithScale1(UWORD x, UWORD y, std::vector<std::vector<uint8_t>>& fontVector,
    unsigned char Color_Foreground, unsigned char Color_Background)
{
    int i, j;
    for(uint8_t font_i = 0; font_i < fontVector.size(); font_i++) 
    {
        const unsigned char* ptr = fontVector[font_i].data();
        for (j = 0; j < font_Height; j++) 
        {
            for (i = 0; i < font_Width; i++) 
            {
                int is_foreground = (*ptr & (0x80 >> (i % 8))) ? 1 : 0;
                if (is_foreground) {
                    SetPixel_1(x + i, y + j, Color_Foreground);
                } else if (FONT_BACKGROUND == Color_Background) {
                    SetPixel_1(x + i, y + j, Color_Background);
                }
                if (i % 8 == 7) ptr++;
            }
            if (font_Width % 8 != 0) ptr++;
        }
        x += font_Width;
    }
}



/***
 * 函数功能：重载字体传入的方式
 */
void EPaper::DrawFont(UWORD x, UWORD y, std::vector<std::vector<uint8_t>>& fontVector,
            unsigned char Color_Foreground, unsigned char Color_Background, 
            SetPixelFuncWrapper& setPixel)
{
    int scale = font_scale;  // 放大倍数
    if (scale <= 1) 
    {
        // 1倍放大无需插值，直接使用原逻辑
        drawFontWithScale1(x, y, fontVector, Color_Foreground, Color_Background);
        return;
    }

    /* 双线性插值放大核心逻辑 */
    for(uint8_t font_i = 0; font_i < fontVector.size(); font_i++) 
    {
        const std::vector<uint8_t>& fontData = fontVector[font_i];
        // 遍历放大后的每个像素点
        for (int dy = 0; dy < font_Height * scale; dy++) 
        {
            for (int dx = 0; dx < font_Width * scale; dx++) 
            {
                // 1. 计算当前放大像素对应原字体的坐标（浮点数）
                float originalX = (float)dx / scale;  // 原X坐标（可能为小数）
                float originalY = (float)dy / scale;  // 原Y坐标（可能为小数）

                // 2. 找到原坐标周围的4个参考像素（整数坐标）
                int x0 = (int)originalX;       // 左方像素X
                int x1 = min(x0 + 1, font_Width - 1);  // 右方像素X（防止越界）
                int y0 = (int)originalY;       // 上方像素Y
                int y1 = min(y0 + 1, font_Height - 1);  // 下方像素Y（防止越界）

                // 3. 获取4个参考像素的灰度值（0为背景，1为前景）
                int pixel00 = getPixelValue(fontData, x0, y0);
                int pixel01 = getPixelValue(fontData, x0, y1);
                int pixel10 = getPixelValue(fontData, x1, y0);
                int pixel11 = getPixelValue(fontData, x1, y1);

                // 4. 计算插值权重（小数部分）
                float wx = originalX - x0;  // X方向权重（0~1）
                float wy = originalY - y0;  // Y方向权重（0~1）

                // 5. 双线性插值计算当前像素值
                float interpolated = 
                    (1 - wx) * (1 - wy) * pixel00 +
                    (1 - wx) * wy * pixel01 +
                    wx * (1 - wy) * pixel10 +
                    wx * wy * pixel11;

                // 6. 根据插值结果绘制像素（阈值0.5判断前景/背景）
                int draw_x = x + dx;
                int draw_y = y + dy;
                if (interpolated >= 0.5) 
                {
                    setPixel(draw_x, draw_y, Color_Foreground);
                } else if (FONT_BACKGROUND == Color_Background) 
                {
                    setPixel(draw_x, draw_y, Color_Background);
                }
            }
        }
        // 字符间距调整
        x += font_space;  
    }
}



/***
 * 函数功能：向左半个屏幕发送数据
 */
void EPaper::sendImage_1(UBYTE* Image)
{
    UWORD Width, Height;
    // Width = (EPD_10in85g_WIDTH % 4 == 0)? (EPD_10in85g_WIDTH / 4 ): (EPD_10in85g_WIDTH / 4 + 1);
    // Height = EPD_10in85g_HEIGHT;
    Width = (EPD_10in85_WIDTH % 4 == 0)? (EPD_10in85_WIDTH / 4 ): (EPD_10in85_WIDTH / 4 + 1);
    Height = EPD_10in85_HEIGHT;
	
    // EPD_10in85g_SendCommand_0(0x10)
    EPD_10in85_SendCommand_0(0x10);
    for (UWORD j = 0; j < Height; j++) 
    {
        for (UWORD i = 0; i < Width; i++) 
        {
            // EPD_10in85g_SendData_0(Image[j*Width + i]);
            EPD_10in85_SendData_0(Image[j*Width + i]);
        }
    }	
}



/***
 * 函数功能：向右半个屏幕发送数据
 */
void EPaper::sendImage_2(UBYTE* Image)
{
    UWORD Width, Height;
    //Width = (EPD_10in85g_WIDTH % 4 == 0)? (EPD_10in85g_WIDTH / 4 ): (EPD_10in85g_WIDTH / 4 + 1);
    Width = (EPD_10in85_WIDTH % 4 == 0)? (EPD_10in85_WIDTH / 4 ): (EPD_10in85_WIDTH / 4 + 1);
    Height = height_;
    // EPD_10in85g_SendCommand_1(0x10);
    EPD_10in85_SendCommand_1(0x10);
    for (UWORD j = 0; j < Height; j++) 
    {
        for (UWORD i = 0; i < Width; i++) 
        {
            // EPD_10in85g_SendData_1(Image[j*Width + i]);
            EPD_10in85_SendData_1(Image[j*Width + i]);
        }
    }
}


/***
 * 函数功能：更新图像到屏幕中
 */
void EPaper::Display(UWORD Xstart, UWORD Ystart, std::vector<std::vector<uint8_t>>& font, 
                    UWORD Color_Foreground, 
                    UWORD font_BackColor, UWORD screen_BackColor)
{
    int font_num = font.size();
    switch(font_num)
    {
        case 2:
        {
            // Xstart = (width_ - font_Width * font_scale) / 2;
            Xstart = 0;
            font_space = height_;

            // 绘制左侧屏幕
            Paint_Clear(screen_BackColor); 
            DrawFont(Xstart, Ystart, font, Color_Foreground, font_BackColor, wrapper1);
            sendImage_1(Paint.Image);

            // 绘制右侧屏幕
            Paint_Clear(screen_BackColor); 
            DrawFont(Xstart, Ystart, font, Color_Foreground, font_BackColor, wrapper2);
            sendImage_2(Paint.Image);
            EPD_10in85_TurnOnDisplay();	
            // EPD_10in85g_TurnOnDisplay();
        }
        break;
        case 3:
        {
            TextSpacing singleLine = calcSingleLineSpacing(
            width_,                     // 屏幕宽度（横向显示）
            font_Width * font_scale,  // 文字宽度
            3,                        // 文字数量
            10                        // 边缘留白
            );
            if (singleLine.success) 
            {
                Serial.print("单行显示 - 均匀间隔：");
                Serial.print(singleLine.spacing);
                Serial.println(" 像素");
                font_space = font_Width * font_scale + singleLine.spacing;
            } else 
            {
                Serial.println("单行显示 - 屏幕空间不足！");
            } 

            // 绘制左侧屏幕
            Paint_Clear(screen_BackColor); 
            DrawFont(Xstart, Ystart, font, Color_Foreground, font_BackColor, wrapper1);
            sendImage_1(Paint.Image);

            // 绘制右侧屏幕
            Paint_Clear(screen_BackColor); 
            DrawFont(Xstart, Ystart, font, Color_Foreground, font_BackColor, wrapper2);
            sendImage_2(Paint.Image);
            EPD_10in85_TurnOnDisplay();
            // EPD_10in85g_TurnOnDisplay();
        };
        break;
        case 4:
        {
            TextSpacing singleLine = calcSingleLineSpacing(
            width_,    // 屏幕宽度（横向显示）
            font_Width * font_scale,  // 文字宽度
            4,                        // 文字数量
            10                        // 边缘留白
            );
            if (singleLine.success) 
            {
                Serial.print("单行显示 - 均匀间隔：");
                Serial.print(singleLine.spacing);
                Serial.println(" 像素");
                font_space = font_Width * font_scale + singleLine.spacing;
            } else 
            {
                Serial.println("单行显示 - 屏幕空间不足！");
            } 
        }
        break;
        case 5:
        break;
    }
}



/***
 * 函数功能：电子纸的初始化
 * 最近修改：2025.9.14
 */
void EPaper::EPaper_Init()
{
    // // 硬件模块初始化
    // DEV_Module_Init();

    // Debug("e-Paper Init and Clear...\r\n");
    // // 屏幕初始化并清屏
    // // EPD_10in85g_Init();
    // EPD_10in85_Init();
    // Debug("e-Paper Init End!\r\n");
    // // EPD_10in85g_Clear(EPD_10in85g_WHITE);
    // EPD_10in85_Clear();
    // Debug("Clear End!\r\n");
    // DEV_Delay_ms(500);
    // EPaper_Sleep();

    // //创建屏幕缓存
    // //UDOUBLE Imagesize = ((EPD_10in85g_WIDTH % 4 == 0)? (EPD_10in85g_WIDTH / 4 ): (EPD_10in85g_WIDTH / 4 + 1)) * EPD_10in85g_HEIGHT;
    // UDOUBLE Imagesize = ((EPD_10in85_WIDTH % 4 == 0)? (EPD_10in85_WIDTH / 4 ): (EPD_10in85_WIDTH / 4 + 1)) * EPD_10in85_HEIGHT;
    // if((Image = (UBYTE *)malloc(Imagesize)) == NULL) 
    // {
    //     Debug("Failed to apply for black memory...\r\n");
    //     while (1);
    // }

    // // 初始化绘图区域
    // Debug("Paint_NewImage\r\n");
    // // Paint_NewImage(Image, EPD_10in85g_WIDTH, EPD_10in85g_HEIGHT, 0, WHITE);
    // Paint_NewImage(Image, EPD_10in85_WIDTH, EPD_10in85_HEIGHT, 0, WHITE);
    // Paint_SetScale(4);
    // Paint_SelectImage(Image);

    Debug("EPD_10IN85_test Demo\r\n");
    DEV_Module_Init();

    Debug("e-Paper Init and Clear...\r\n");
    EPD_10in85_Init();
    EPD_10in85_Clear();
    DEV_Delay_ms(500);

    //Create a new image cache
    UDOUBLE Imagesize = ((EPD_10in85_WIDTH % 8 == 0)? (EPD_10in85_WIDTH / 8 ): (EPD_10in85_WIDTH / 8 + 1)) * EPD_10in85_HEIGHT;
    if((Image = (UBYTE *)malloc(Imagesize)) == NULL) {
        Debug("Failed to apply for black memory...\r\n");
        while (1);
    }
    Debug("Paint_NewImage\r\n");
    Paint_NewImage(Image, EPD_10in85_WIDTH*2, EPD_10in85_HEIGHT, 0, WHITE);
    Paint_SetScale(2);
    Paint_SelectImage(Image);
    Paint_Clear(WHITE);
}



/***
 * 函数功能：电子纸关闭
 * 最近修改：2025.9.14
 */
void EPaper::EPaper_Close()
{
    Debug("Clear...\r\n");
    // EPD_10in85g_Init();
    // EPD_10in85g_Clear(EPD_10in85g_WHITE);
    EPD_10in85_Init();
    EPD_10in85_Clear();

    // 进入休眠模式
    Debug("Goto Sleep...\r\n");
    // EPD_10in85g_Sleep();
    EPD_10in85_Sleep();
    free(Image);
    DEV_Delay_ms(2000);

    // 关闭电源
    Debug("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
}



/***
 * 函数功能：屏幕睡眠
 * 最近修改：2025.9.22
 */
void EPaper::EPaper_Sleep()
{
    Debug("Goto Sleep...\r\n");
    // EPD_10in85g_Sleep();
    EPD_10in85_Sleep();
    DEV_Delay_ms(2000);
}



/***
 * 函数功能：屏幕苏醒
 * 最近修改：2025.9.22
 */
void EPaper::EPaper_DisSleep()
{
    // EPD_10in85g_Init();
    EPD_10in85_Init();
}
/*************************************************************/
