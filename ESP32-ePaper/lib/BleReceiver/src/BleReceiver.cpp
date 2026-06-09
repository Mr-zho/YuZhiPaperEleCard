// #include "BleReceiver.h"
// #include <BLE2902.h>

// #define SERVICE_UUID   "0000FFF0-0000-1000-8000-00805F9B34FB"
// #define CHAR_UUID_TX   "0000FFF1-0000-1000-8000-00805F9B34FB"
// #define CHAR_UUID_RX   "0000FFF2-0000-1000-8000-00805F9B34FB"

// void BleReceiver::begin(const char* name)
// {
//     BLEDevice::init(name);

//     BLEServer* server = BLEDevice::createServer();
//     BLEService* service = server->createService(SERVICE_UUID);

//     rxChar = service->createCharacteristic(
//                 CHAR_UUID_RX,
//                 BLECharacteristic::PROPERTY_WRITE | 
//                 BLECharacteristic::PROPERTY_WRITE_NR // 提高传输效率
//             );

//     // TX: 设备主动通知手机数据更新
//     auto txChar = service->createCharacteristic(
//                 CHAR_UUID_TX,
//                 BLECharacteristic::PROPERTY_NOTIFY | // 开启通知模式（最核心）
//                 BLECharacteristic::PROPERTY_READ     // 可选，允许手机手动读取
//             );
    
//     txChar->addDescriptor(new BLE2902()); // 这个描述符是必须的，负责通知开关的状态

//     rxChar->setCallbacks(new RxCallbacks(this));

//     service->start();
//     BLEAdvertising* advertising = BLEDevice::getAdvertising();
//     advertising->start();

//     Serial.println("BLE Ready");
// }


// void BleReceiver::dispatch(uint8_t type,
//                            const uint8_t* payload,
//                            size_t len)
// {
//     handleText(type, payload, len);
//     handleImage(type, payload, len);
// }


// // ============================
// // 用户实现逻辑
// // ============================
// void BleReceiver::onTextReceived(const String& text)
// {
//     Serial.print("Text received: ");
//     Serial.println(text);
//     // 回调触发
//     if (textCallback) {
//         textCallback(text);
//     }
// }

// void BleReceiver::onImageReceived(const uint8_t* img,
//                                   uint16_t w,
//                                   uint16_t h)
// {
//     Serial.printf("IMAGE: %dx%d\n", w, h);
//     uint32_t imageSize = (w * h) / 4; // 2bit

//     Serial.printf("[IMAGE] %dx%d size=%d\n", w, h, imageSize);

//     // 回调触发
//     if (imageCallback) {
//         imageCallback(img, imageSize);
//     }
// }
