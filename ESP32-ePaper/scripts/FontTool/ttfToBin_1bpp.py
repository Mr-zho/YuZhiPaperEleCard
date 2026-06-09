"""
将 TTF 转换为：
1. GB2312 编码、横向取模点阵字库（.bin）
2. Unicode → GB2312 映射表（.map）

取模规则：
- 横向取模（行优先）
- 高位在前
- 1 = 显示（黑），0 = 不显示（白）
- 基线对齐，字符在方块中垂直居中
"""

import sys
import struct
import os
from PIL import Image, ImageDraw, ImageFont

ENCODING = "gb2312"

# GB2312 编码范围
GB2312_ZONE_START = 0xA1
GB2312_ZONE_END   = 0xF7
GB2312_BIT_START  = 0xA1
GB2312_BIT_END    = 0xFE

TOTAL_CHARS = (GB2312_ZONE_END - GB2312_ZONE_START + 1) * \
              (GB2312_BIT_END - GB2312_BIT_START + 1)


def char_to_bitmap(char, font, width, height):
    """单字符取模（横向取模，高位在前，基线对齐）"""

    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)

    try:
        ascent, descent = font.getmetrics()

        # 计算基线位置，让字垂直居中
        baseline_y = (height + ascent - descent) // 2
        draw_y = baseline_y - ascent

        draw.text((0, draw_y), char, font=font, fill=0)
    except Exception:
        return None

    bitmap = bytearray()

    for y in range(height):
        cur_byte = 0
        bit_count = 0

        for x in range(width):
            pixel = img.getpixel((x, y))
            pixel_val = 1 if pixel == 0 else 0  # 1=黑

            cur_byte = (cur_byte << 1) | pixel_val
            bit_count += 1

            if bit_count == 8:
                bitmap.append(cur_byte)
                cur_byte = 0
                bit_count = 0

        if bit_count != 0:
            cur_byte <<= (8 - bit_count)
            bitmap.append(cur_byte)

    return bytes(bitmap)


def main():
    if len(sys.argv) != 4:
        print("用法：python tftToBin.py <font.ttf> <width> <height>")
        sys.exit(1)

    font_file = sys.argv[1]
    width = int(sys.argv[2])
    height = int(sys.argv[3])

    font_size = max(width, height)
    bytes_per_char = (width * height + 7) // 8

    base_name = os.path.splitext(os.path.basename(font_file))[0]
    output_bin = f"{base_name}_{width}x{height}.bin"
    output_map = f"{base_name}_{width}x{height}_unicode_gb2312.map"

    print("字体文件:", font_file)
    print("点阵尺寸:", f"{width} x {height}")
    print("单字节数:", bytes_per_char)
    print("字库文件:", output_bin)
    print("映射文件:", output_map)

    try:
        font = ImageFont.truetype(font_file, font_size, encoding=ENCODING)
    except Exception as e:
        print("字体加载失败:", e)
        sys.exit(1)

    # 预分配字库文件
    with open(output_bin, "wb") as f:
        f.write(b'\x00' * TOTAL_CHARS * bytes_per_char)

    count = 0

    with open(output_bin, "r+b") as fbin, open(output_map, "wb") as fmap:
        for zone in range(GB2312_ZONE_START, GB2312_ZONE_END + 1):
            for bit in range(GB2312_BIT_START, GB2312_BIT_END + 1):
                try:
                    gb_bytes = struct.pack('BB', zone, bit)
                    char = gb_bytes.decode(ENCODING)
                except UnicodeDecodeError:
                    continue

                bitmap = char_to_bitmap(char, font, width, height)
                if not bitmap:
                    continue

                index = ((zone - GB2312_ZONE_START) *
                         (GB2312_BIT_END - GB2312_BIT_START + 1) +
                         (bit - GB2312_BIT_START))

                # 写点阵
                fbin.seek(index * bytes_per_char)
                fbin.write(bitmap)

                # 写 Unicode → GB2312 映射
                unicode_val = ord(char)
                gb2312_val = (zone << 8) | bit
                fmap.write(struct.pack(">I H", unicode_val, gb2312_val))

                count += 1
                if count % 500 == 0:
                    print(f"已处理 {count}/{TOTAL_CHARS}")

    print("\n转换完成")
    print("有效字符数:", count)
    print("字库大小:", os.path.getsize(output_bin), "字节")
    print("映射表大小:", os.path.getsize(output_map), "字节")


if __name__ == "__main__":
    main()
