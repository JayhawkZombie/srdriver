#pragma once

// Platform detection
#ifdef ARDUINO_NANO_ESP32
    #define PLATFORM_ESP32_S3 1
    #define SUPPORTS_BLE 1
    #define SUPPORTS_WIFI 0  // Disabled WiFi for ESP32
    #define SUPPORTS_DISPLAY 1
    #define SUPPORTS_SD_CARD 1
    #define SUPPORTS_PREFERENCES 1
    #define SUPPORTS_ESP32_APIS 1
    #define SUPPORTS_TEMPERATURE_SENSOR 1
#elif defined(ARDUINO_RASPBERRY_PI_PICO)
    #define PLATFORM_RP2040 1
    #define SUPPORTS_BLE 1
    #define SUPPORTS_WIFI 0  // No built-in WiFi
    #define SUPPORTS_DISPLAY 1
    #define SUPPORTS_SD_CARD 0  // Disable SD card for RP2040 LTS
    #define SUPPORTS_PREFERENCES 0  // Use SD card instead
    #define SUPPORTS_ESP32_APIS 0
    #define SUPPORTS_TEMPERATURE_SENSOR 0
#else
    #error "Unsupported platform - please add platform detection"
#endif

// Feature availability checks
inline bool PlatformSupportsBLE() {
    #if SUPPORTS_BLE
        return true;
    #else
        return false;
    #endif
}

inline bool PlatformSupportsWiFi() {
    #if SUPPORTS_WIFI
        return true;
    #else
        return false;
    #endif
}

inline bool PlatformSupportsDisplay() {
    #if SUPPORTS_DISPLAY
        return true;
    #else
        return false;
    #endif
}

inline bool PlatformSupportsSDCard() {
    #if SUPPORTS_SD_CARD
        return true;
    #else
        return false;
    #endif
}

inline bool PlatformSupportsPreferences() {
    #if SUPPORTS_PREFERENCES
        return true;
    #else
        return false;
    #endif
}

inline bool PlatformSupportsESP32APIs() {
    #if SUPPORTS_ESP32_APIS
        return true;
    #else
        return false;
    #endif
} 

inline bool PlatformSupportsTemperatureSensor() {
    #if SUPPORTS_TEMPERATURE_SENSOR
        return true;
    #else
        return false;
    #endif
}