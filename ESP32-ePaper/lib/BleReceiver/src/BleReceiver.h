// #pragma once
// #include <BLEDevice.h>
// #include <BLEServer.h>
// #include <BLEUtils.h>

// #include "Proto.h"
// #include "Capability.h"

// class BleReceiver :
//         public ProtoBase,
//         public TextCapability,
//         public ImageCapability
// {
// public:
//     void begin(const char* name = "ESP32-BLE");
//     // 图像回调注册
//     void setImageCallback(std::function<void(const uint8_t*, uint16_t)> cb)
//     {
//         imageCallback = cb;
//     }
//     void setTextCallback(std::function<void(const String& text)> cb)
//     {
//         textCallback = cb;
//     }

// protected:
//     void dispatch(uint8_t type,
//                   const uint8_t* payload,
//                   size_t len) override;

//     void onTextReceived(const String& text) override;
//     void onImageReceived(const uint8_t* img,
//                          uint16_t w,
//                          uint16_t h) override;

// private:
//     BLECharacteristic* rxChar;
//     std::function<void(const uint8_t*, uint16_t)> imageCallback = nullptr;
//     std::function<void(const String& text)> textCallback = nullptr;

//     class RxCallbacks : public BLECharacteristicCallbacks {
//     public:
//         RxCallbacks(BleReceiver* p) : parent(p) {}

//         void onWrite(BLECharacteristic* c) override {
//             String v = c->getValue();

//             if (!v.isEmpty()) {
//                 parent->input((uint8_t*)v.c_str(), v.length());
//             }
//         }

//         void onConnect(BLEServer* pServer) {
//             Serial.println("Device Connected");
//         };

//         void onDisconnect(BLEServer* pServer) {
//             Serial.println("Device Disconnected. Restarting advertising...");
//             // 关键：当断开连接时，让设备重新进入广播状态，这样手机才能再次搜到它
//             pServer->getAdvertising()->start();
//         }

//     private:
//         BleReceiver* parent;
//     };
// };
#pragma once

#include <NimBLEDevice.h>
#include "Proto.h"
#include "Capability.h"

#define SERVICE_UUID   "0000FFF0-0000-1000-8000-00805F9B34FB"
#define CHAR_UUID_TX   "0000FFF1-0000-1000-8000-00805F9B34FB"
#define CHAR_UUID_RX   "0000FFF2-0000-1000-8000-00805F9B34FB"

class BleReceiver :
        public ProtoBase,
        public TextCapability,
        public ImageCapability
{
public:
    void begin(const char* name = "ESP32-BLE")
    {
        NimBLEDevice::init(name);

        NimBLEServer* server = NimBLEDevice::createServer();
        server->setCallbacks(new ServerCallbacks()); 

        NimBLEService* service = server->createService(SERVICE_UUID);

        // RX 特征
        rxChar = service->createCharacteristic(
                    CHAR_UUID_RX,
                    NIMBLE_PROPERTY::WRITE | 
                    NIMBLE_PROPERTY::WRITE_NR // 提高传输效率
                );
        rxChar->setCallbacks(new RxCallbacks(this));

        // TX 特征
        service->createCharacteristic(
                    CHAR_UUID_TX,
                    NIMBLE_PROPERTY::NOTIFY |
                    NIMBLE_PROPERTY::READ
                );

        NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
        
        // 1. 设置主广告数据（广播名字和标志位）
        NimBLEAdvertisementData advData;
        advData.setName(name);
    
        advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP); 
        advertising->setAdvertisementData(advData);

        // 2. 设置扫描回复数据（将 128-bit 长 UUID 塞进回复包）
        NimBLEAdvertisementData scanResponseData;

        scanResponseData.addServiceUUID(NimBLEUUID(SERVICE_UUID));
        advertising->setScanResponseData(scanResponseData);

        // 3. 启动广播
        advertising->start();

        Serial.println("BLE Ready and Advertising...");
    }

    // 图像回调注册
    void setImageCallback(std::function<void(const uint8_t*, uint16_t)> cb)
    {
        imageCallback = cb;
    }

    void setTextCallback(std::function<void(const String& text)> cb)
    {
        textCallback = cb;
    }

protected:
    void dispatch(uint8_t type,
                  const uint8_t* payload,
                  size_t len) override
    {
        if(type == TYPE_TEXT) {
            handleText(type, payload, len);
        } else if (type == TYPE_IMAGE) {
            handleImage(type, payload, len);
        } else {
            Serial.printf("Unknown type: %d\n", type);
        }
    }

    void onTextReceived(const String& text) override
    {
        Serial.print("Text received: ");
        Serial.println(text);
        if (textCallback) {
            textCallback(text);
        }
    }

    void onImageReceived(const uint8_t* img,
                         uint16_t w,
                         uint16_t h) override
    {
        Serial.printf("IMAGE: %dx%d\n", w, h);
        uint32_t imageSize = (w * h) / 4; // 2bit
        Serial.printf("[IMAGE] %dx%d size=%d\n", w, h, imageSize);

        if (imageCallback) {
            imageCallback(img, imageSize);
        }
    }

private:
    NimBLECharacteristic* rxChar;
    std::function<void(const uint8_t*, uint16_t)> imageCallback = nullptr;
    std::function<void(const String& text)> textCallback = nullptr;

    // 1. 适配 NimBLE 2.x 的服务器状态回调
    class ServerCallbacks : public NimBLEServerCallbacks {
        // 对应：virtual void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo);
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override 
        {
            Serial.println("Device Connected");
        }

        // 对应：virtual void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason);
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override 
        {
            Serial.printf("Device Disconnected. Reason: %d. Restarting advertising...\n", reason);
            NimBLEDevice::startAdvertising(); // 重新开始广播
        }
    };

    // 2. 适配 NimBLE 2.x 的特征数据接收回调
    class RxCallbacks : public NimBLECharacteristicCallbacks {
    public:
        RxCallbacks(BleReceiver* p) : parent(p) {}

        // 新版 NimBLE 推荐的包含 connInfo 的完整重载签名，确保 override 能够匹配成功
        void onWrite(NimBLECharacteristic* c, NimBLEConnInfo& connInfo) override
        {
            const uint8_t* data = c->getValue().data();
            size_t length = c->getValue().length();

            if (length > 0) {
                // 直接传递底层指针，避免数据拷贝
                parent->input(data, length);
            }
        }

    private:
        BleReceiver* parent;
    };
};