#pragma once

#ifdef ARDUINO_NANO_ESP32

#include <ESP.h>
#include "../PlatformFactory.h"
#include "ESP32SDCardController.h"

class ESP32PlatformFactory : public PlatformFactory {
public:
    // System information (ESP32-specific)
    static uint32_t getFreeHeap() { return ESP.getFreeHeap(); }
    static uint32_t getHeapSize() { return ESP.getHeapSize(); }
    static uint32_t getCpuFreqMHz() { return ESP.getCpuFreqMHz(); }
    static uint32_t getMinFreeHeap() { return ESP.getMinFreeHeap(); }
    // HAL factory methods (ESP32-specific)
    static SDCardController* createSDCardController() { return new ESP32SDCardController(); }
};

#endif // ARDUINO_NANO_ESP32 