#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <hal/PlatformFactory.h>

extern SDCardController* g_sdCardController;

class JsonSettings
{
public:

    JsonSettings(const char* filename);


    bool load();
    bool getBool(const String& path, bool defaultValue = false) const;
    int getInt(const String& path, int defaultValue = 0) const;
    uint8_t getUint8(const String& path, uint8_t defaultValue = 0) const;
    uint16_t getUint16(const String& path, uint16_t defaultValue = 0) const;
    uint32_t getUint32(const String& path, uint32_t defaultValue = 0) const;
    float getFloat(const String& path, float defaultValue = 0.0f) const;
    uint8_t getHexUint8(const String& path, uint8_t defaultValue = 0) const;
    uint16_t getHexUint16(const String& path, uint16_t defaultValue = 0) const;
    DynamicJsonDocument _doc;


private:

    String getString(const String& path, const char* defaultValue = nullptr) const;

    String _filename;


};

