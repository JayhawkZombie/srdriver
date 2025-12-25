#include "DeviceInfo.h"

String DeviceInfo::_deviceName = DEVICE_NAME;
String DeviceInfo::_firmwareVersion = FIRMWARE_VERSION;
String DeviceInfo::_buildDate = BUILD_DATE;
String DeviceInfo::_buildTime = BUILD_TIME;
String DeviceInfo::_buildTimestamp = BUILD_TIMESTAMP;
String DeviceInfo::_deviceVersion = DEVICE_VERSION;
std::vector<String> DeviceInfo::_capabilities = {
    #if SUPPORTS_LEDS
    "LEDS"
    #endif // SUPPORTS_LEDS

    #if SUPPORTS_BLE
    , "BLE"
    #endif // SUPPORTS_BLE

    #if SUPPORTS_WIFI
    , "WIFI"
    #endif // SUPPORTS_WIFI

    #if SUPPORTS_DISPLAY
    , "DISPLAY"
    #endif // SUPPORTS_DISPLAY

    #if SUPPORTS_SD_CARD
    , "SD_CARD"
    #endif // SUPPORTS_SD_CARD

    #if SUPPORTS_TEMPERATURE_SENSOR
    , "TEMPERATURE_SENSOR"
    #endif // SUPPORTS_TEMPERATURE_SENSOR

    #if SUPPORTS_POWER_SENSORS
    , "POWER_SENSORS"
    #endif // SUPPORTS_POWER_SENSORS

    #if SUPPORTS_ROTARY_ENCODER
    , "ROTARY_ENCODER"
    #endif // SUPPORTS_ROTARY_ENCODER

    #if SUPPORTS_ESP32_APIS
    , "ESP32_APIS"
    #endif // SUPPORTS_ESP32_APIS
};
String DeviceInfo::_versionBranch = VERSION_BRANCH;
String DeviceInfo::_versionHash = VERSION_HASH;
String DeviceInfo::_versionTag = VERSION_TAG;
