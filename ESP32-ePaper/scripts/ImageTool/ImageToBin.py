# #!/usr/bin/env python3
# import cv2
# import numpy as np
# import argparse

# # ===================== 屏幕参数（与驱动严格一致） =====================
# SCREEN_WIDTH  = 128
# SCREEN_HEIGHT = 296

# # ===================== 判色参数 =====================
# RED_R_MIN = 120
# RED_RATIO = 1.3
# BLACK_LUMA_THRESHOLD = 128


# # ==================================================
# def rotate_image(img, rotate):
#     if rotate == 90:
#         return cv2.rotate(img, cv2.ROTATE_90_CLOCKWISE)
#     elif rotate == 180:
#         return cv2.rotate(img, cv2.ROTATE_180)
#     elif rotate == 270:
#         return cv2.rotate(img, cv2.ROTATE_90_COUNTERCLOCKWISE)
#     return img


# # ==================================================
# def image_to_epd_buffers(image_path, rotate=270):
#     """
#     输出：
#         black_buf, red_buf
#     格式：
#         - 128 x 296
#         - 行优先
#         - MSB 在左
#         - 与 EPD_2IN9B_V4_Display_Fast 完全对齐
#     """

#     img = cv2.imread(image_path)
#     if img is None:
#         raise FileNotFoundError(f"无法读取图像：{image_path}")

#     img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

#     # ---------- 一次性旋转 ----------
#     img = rotate_image(img, rotate)

#     # ---------- 等比缩放 ----------
#     h, w = img.shape[:2]
#     scale = max(SCREEN_WIDTH / w, SCREEN_HEIGHT / h)
#     img = cv2.resize(
#         img,
#         (int(w * scale), int(h * scale)),
#         interpolation=cv2.INTER_AREA
#     )

#     # ---------- 居中裁剪 ----------
#     if img.shape[1] < SCREEN_WIDTH or img.shape[0] < SCREEN_HEIGHT:
#         raise RuntimeError(
#             f"图像尺寸不足：{img.shape[1]}x{img.shape[0]}，"
#             f"无法裁剪为 {SCREEN_WIDTH}x{SCREEN_HEIGHT}"
#         )

#     sx = (img.shape[1] - SCREEN_WIDTH) // 2
#     sy = (img.shape[0] - SCREEN_HEIGHT) // 2
#     img = img[sy:sy + SCREEN_HEIGHT, sx:sx + SCREEN_WIDTH]

#     # ---------- 缓冲区 ----------
#     byte_width = SCREEN_WIDTH // 8  # 16 bytes per row
#     black = np.zeros((SCREEN_HEIGHT, byte_width), dtype=np.uint8)
#     red   = np.zeros((SCREEN_HEIGHT, byte_width), dtype=np.uint8)

#     # ---------- 像素映射 ----------
#     for y in range(SCREEN_HEIGHT):
#         for x in range(SCREEN_WIDTH):
#             r, g, b = img[y, x]
#             byte = x >> 3
#             bit  = 7 - (x & 7)

#             is_red = (
#                 r > RED_R_MIN and
#                 r > g * RED_RATIO and
#                 r > b * RED_RATIO
#             )

#             luma = 0.299 * r + 0.587 * g + 0.114 * b
#             is_black = luma < BLACK_LUMA_THRESHOLD

#             if is_red:
#                 red[y, byte] |= (1 << bit)
#             elif is_black:
#                 black[y, byte] |= (1 << bit)

#     return black.flatten(), red.flatten()


# # ==================================================
# def save_bin(path, data):
#     with open(path, "wb") as f:
#         f.write(data.tobytes())
#     print(f"[OK] BIN 保存：{path} ({len(data)} bytes)")


# def main():
#     parser = argparse.ArgumentParser(
#         description="EPD 2.9\" (128x296) Black/Red Image Converter"
#     )
#     parser.add_argument("image", help="输入图片路径")
#     parser.add_argument(
#         "--rotate",
#         type=int,
#         choices=[0, 90, 180, 270],
#         default=270,          # ★ 默认 270°
#         help="顺时针旋转角度（默认 270）"
#     )
#     parser.add_argument(
#         "-o", "--out",
#         default="image",
#         help="输出文件名前缀"
#     )

#     args = parser.parse_args()

#     black, red = image_to_epd_buffers(
#         args.image,
#         rotate=args.rotate
#     )

#     # ★ 文件名互换（内容不换）
#     save_bin(args.out + "_black.bin", red)
#     save_bin(args.out + "_red.bin",   black)


# if __name__ == "__main__":
#     main()
 

#!/usr/bin/env python3
import cv2
import numpy as np
import argparse

# ===================== 屏幕参数 =====================
SCREEN_WIDTH  = 128
SCREEN_HEIGHT = 296

# ===================== 判色参数 =====================
RED_R_MIN = 120
RED_RATIO = 1.3
BLACK_LUMA_THRESHOLD = 128


# ==================================================
def rotate_image(img, rotate):
    if rotate == 90:
        return cv2.rotate(img, cv2.ROTATE_90_CLOCKWISE)
    elif rotate == 180:
        return cv2.rotate(img, cv2.ROTATE_180)
    elif rotate == 270:
        return cv2.rotate(img, cv2.ROTATE_90_COUNTERCLOCKWISE)
    return img


# ==================================================
def image_to_epd_buffers(image_path, rotate=270):
    img = cv2.imread(image_path)
    if img is None:
        raise FileNotFoundError(f"无法读取图像：{image_path}")

    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    # ---------- 旋转 ----------
    img = rotate_image(img, rotate)

    # ---------- 缩放 ----------
    h, w = img.shape[:2]
    scale = max(SCREEN_WIDTH / w, SCREEN_HEIGHT / h)
    img = cv2.resize(img, (int(w * scale), int(h * scale)),
                     interpolation=cv2.INTER_AREA)

    # ---------- 裁剪 ----------
    sx = (img.shape[1] - SCREEN_WIDTH) // 2
    sy = (img.shape[0] - SCREEN_HEIGHT) // 2
    img = img[sy:sy + SCREEN_HEIGHT, sx:sx + SCREEN_WIDTH]

    # ---------- 缓冲区 ----------
    byte_width = SCREEN_WIDTH // 8
    black = np.full((SCREEN_HEIGHT, byte_width), 0xFF, dtype=np.uint8)

    # ⚠️ 关键：红层默认全 1（白）
    red = np.full((SCREEN_HEIGHT, byte_width), 0xFF, dtype=np.uint8)

    # ---------- 取模 ----------
    for y in range(SCREEN_HEIGHT):
        for x in range(SCREEN_WIDTH):
            r, g, b = img[y, x]
            byte = x >> 3
            bit  = 7 - (x & 7)

            is_red = (
                r > RED_R_MIN and
                r > g * RED_RATIO and
                r > b * RED_RATIO
            )

            luma = 0.299 * r + 0.587 * g + 0.114 * b
            is_black = luma < BLACK_LUMA_THRESHOLD

            if is_red:
                # 红色 → ryimage 清 0
                red[y, byte] &= ~(1 << bit)
            elif is_black:
                # 黑色 → black 清0
                black[y, byte] &= ~(1 << bit)
            # 白色：black=0, red=1（什么都不做）

    return black.flatten(), red.flatten()


# ==================================================
def save_bin(path, data):
    with open(path, "wb") as f:
        f.write(data.tobytes())
    print(f"[OK] BIN 保存：{path} ({len(data)} bytes)")


def main():
    parser = argparse.ArgumentParser(
        description="EPD 2.9\" (128x296) Image Converter (V4 driver aligned)"
    )
    parser.add_argument("image", help="输入图片路径")
    parser.add_argument("--rotate", type=int, choices=[0, 90, 180, 270], default=270)
    parser.add_argument("-o", "--out", default="image")

    args = parser.parse_args()

    black, red = image_to_epd_buffers(args.image, rotate=args.rotate)

    save_bin(args.out + "_black.bin", black)
    save_bin(args.out + "_red.bin",   red)


if __name__ == "__main__":
    main()
