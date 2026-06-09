#pragma once
#include "Proto.h"

class TextCapability {
protected:
    void handleText(uint8_t type,
                    const uint8_t* payload,
                    size_t len)
    {
        if (type != TYPE_TEXT) return;

        String text((const char*)payload, len);
        onTextReceived(text);
    }

    virtual void onTextReceived(const String& text) = 0;
};


class ImageCapability {
protected:
    void handleImage(uint8_t type,
                     const uint8_t* payload,
                     size_t len)
    {
        if (type != TYPE_IMAGE) return;

        if (len < sizeof(ImageHeader)) return;

        const ImageHeader* hdr = (const ImageHeader*)payload;
        const uint8_t* img = payload + sizeof(ImageHeader);

        size_t imgSize = len - sizeof(ImageHeader);

        onImageReceived(img, hdr->width, hdr->height);
    }

    virtual void onImageReceived(const uint8_t* img,
                                 uint16_t w,
                                 uint16_t h) = 0;
};