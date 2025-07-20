#include "JsonSettings.h"
#include <freertos/LogManager.h>

JsonSettings::JsonSettings(const char* filename) : _doc(1024)
{
    _filename = filename;
}

bool JsonSettings::load()
{
    if (g_sdCardController && g_sdCardController->exists(_filename.c_str()))
    {
        String jsonString = g_sdCardController->readFile(_filename.c_str());
        DeserializationError error = deserializeJson(_doc, jsonString);
        if (error)
        {
            LOG_ERRORF("Failed to deserialize JSON: %s", error.c_str());
            return false;
        }
        return true;
    }
    return false;
}

bool JsonSettings::getBool(const String& path, bool defaultValue) const
{
    String value = getString(path, nullptr);
    if (value.length() > 0)
    {
        return value.equalsIgnoreCase("true") || value.equals("1");
    }
    return defaultValue;
}

int JsonSettings::getInt(const String& path, int defaultValue) const
{
    String value = getString(path, nullptr);
    if (value.length() > 0)
    {
        return value.toInt();
    }
    return defaultValue;
}   

uint8_t JsonSettings::getUint8(const String& path, uint8_t defaultValue) const
{
    String value = getString(path, nullptr);
    if (value.length() > 0)
    {
        return value.toInt();
    }
    return defaultValue;
}

uint16_t JsonSettings::getUint16(const String& path, uint16_t defaultValue) const
{
    String value = getString(path, nullptr);
    if (value.length() > 0)
    {
        return value.toInt();
    }
    return defaultValue;
}

uint32_t JsonSettings::getUint32(const String& path, uint32_t defaultValue) const
{
    String value = getString(path, nullptr);
    if (value.length() > 0)
    {
        return value.toInt();
    }
    return defaultValue;
}

uint8_t JsonSettings::getHexUint8(const String& path, uint8_t defaultValue) const
{
    String value = getString(path, nullptr);
    // has to start with 0x
    if (value.length() > 0 && value.startsWith("0x"))
    {
        // has to convert from hex to uint8_t
        return strtoul(value.substring(2).c_str(), nullptr, 16);
    }
    return defaultValue;
}

String JsonSettings::getString(const String& path, const char* defaultValue) const
{
    if (_doc.isNull()) {
        return defaultValue ? defaultValue : String();
    }
    
    // Split the path by dots to handle nested objects
    int dotIndex = path.indexOf('.');
    
    if (dotIndex == -1) {
        // Simple case: no dots, just check the top level
        if (_doc.containsKey(path.c_str()))
        {
            return _doc[path.c_str()].as<String>();
        }
    } else {
        // Nested case: split by dots and traverse
        String currentPath = path.substring(0, dotIndex);
        String remainingPath = path.substring(dotIndex + 1);
        
        // Try direct nested access using bracket notation
        JsonVariantConst nested = _doc[currentPath.c_str()][remainingPath.c_str()];
        if (!nested.isNull()) {
            return nested.as<String>();
        }
    }
    
    return defaultValue ? defaultValue : String();
}

uint16_t JsonSettings::getHexUint16(const String& path, uint16_t defaultValue) const
{
    String value = getString(path, nullptr);
    if (value.length() > 0 && value.startsWith("0x"))
    {
        return strtoul(value.substring(2).c_str(), nullptr, 16);
    }
    return defaultValue;
}
