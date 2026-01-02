#pragma once

#include "PlatformConfig.h"
#include "SDCardController.h"
#include "hal/temperature/DS18B20Component.h"

class PlatformFactory {
public:
    // Platform information
    static const char* getPlatformName() {
        #if PLATFORM_ESP32_S3
            return "ESP32-S3";
        #elif PLATFORM_RP2040
            return "RP2040";
        #elif PLATFORM_CROW_PANEL
            return "Elecrow CrowPanel";
        #else
            return "Unknown";
        #endif
    }
    
    // Feature support checks
    static bool supportsBLE() { return PlatformSupportsBLE(); }
    static bool supportsWiFi() { return PlatformSupportsWiFi(); }
    static bool supportsDisplay() { return PlatformSupportsDisplay(); }
    static bool supportsSDCard() { return PlatformSupportsSDCard(); }
    static bool supportsPreferences() { return PlatformSupportsPreferences(); }
    static bool supportsESP32APIs() { return PlatformSupportsESP32APIs(); }
    static bool supportsTemperatureSensor() { return PlatformSupportsTemperatureSensor(); }
    
    // HAL factory methods
    static SDCardController* createSDCardController();
    static DS18B20Component* createTemperatureSensor(int pin);
    
    // System information (platform-agnostic where possible)
    static uint32_t getFreeHeap() {
        #if SUPPORTS_ESP32_APIS
            return ESP.getFreeHeap();
        #else
            return 0; // TODO: Implement for RP2040
        #endif
    }
    
    static uint32_t getHeapSize() {
        #if SUPPORTS_ESP32_APIS
            return ESP.getHeapSize();
        #else
            return 0; // TODO: Implement for RP2040
        #endif
    }
    
    static uint32_t getCpuFreqMHz() {
        #if SUPPORTS_ESP32_APIS
            return ESP.getCpuFreqMHz();
        #else
            return 133; // RP2040 default
        #endif
    }
    
    static uint32_t getMinFreeHeap() {
        #if SUPPORTS_ESP32_APIS
            return ESP.getMinFreeHeap();
        #else
            return 0; // TODO: Implement for RP2040
        #endif
    }
}; 
