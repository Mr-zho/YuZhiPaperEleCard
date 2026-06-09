#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

// ==================== 配置管理类 ====================
class UserConfigManager
{
private:
    const char* CONFIG_FILE = "/config.json"; // 配置文件路径
    UserConfig cfg;

    // 默认配置
    UserConfig defaultConfig()
    {
        UserConfig c;
        c.birth_year  = 2000;
        c.birth_month = 1;
        c.birth_day   = 1;
        c.wifi_ssid   = "";
        c.wifi_pass   = "";
        return c;
    }

public:
    // ==================== 初始化 ====================
    bool begin()
    {
        if(!LittleFS.begin(true)){
            Serial.println("挂载LittleFS失败");
            return false;
        }

        return load(); // 加载配置
    }

    // ==================== 获取当前配置 ====================
    UserConfig get() const
    {
        return cfg;
    }

    // ==================== 保存当前配置 ====================
    bool save()
    {
        StaticJsonDocument<256> doc;
        doc["birth_year"]  = cfg.birth_year;
        doc["birth_month"] = cfg.birth_month;
        doc["birth_day"]   = cfg.birth_day;
        doc["wifi_ssid"]   = cfg.wifi_ssid;
        doc["wifi_pass"]   = cfg.wifi_pass;

        File file = LittleFS.open(CONFIG_FILE, "w");
        if(!file){
            Serial.println("打开文件失败");
            return false;
        }

        if(serializeJson(doc, file) == 0){
            Serial.println("写入文件失败");
            file.close();
            return false;
        }

        file.close();
        return true;
    }

    // ==================== 加载配置 ====================
    bool load()
    {
        if(!LittleFS.exists(CONFIG_FILE)){
            Serial.println("配置文件不存在，创建默认配置");
            cfg = defaultConfig();
            return save();
        }

        File file = LittleFS.open(CONFIG_FILE, "r");
        if(!file){
            Serial.println("打开文件失败");
            return false;
        }

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if(error){
            Serial.println("JSON解析失败，使用默认配置");
            cfg = defaultConfig();
            return save();
        }

        cfg.birth_year  = doc["birth_year"] | 2000;
        cfg.birth_month = doc["birth_month"] | 1;
        cfg.birth_day   = doc["birth_day"] | 1;
        cfg.wifi_ssid   = doc["wifi_ssid"].as<String>();
        cfg.wifi_pass   = doc["wifi_pass"].as<String>();

        return true;
    }

    // ==================== 修改配置 ====================
    void set(const UserConfig &newCfg)
    {
        cfg = newCfg;
    }

    void setWiFi(const String &ssid, const String &pass)
    {
        cfg.wifi_ssid = ssid;
        cfg.wifi_pass = pass;
    }

    void setBirthDate(int year, int month, int day)
    {
        cfg.birth_year  = year;
        cfg.birth_month = month;
        cfg.birth_day   = day;
    }
};