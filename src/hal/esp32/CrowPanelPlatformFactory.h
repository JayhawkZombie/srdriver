#pragma once

#ifdef PLATFORM_CROW_PANEL

#include <Esp.h>
#include "../PlatformFactory.h"
#include "CrowPanelSDCardController.h"

class CrowPanelPlatformFactory : public PlatformFactory {
public:
    // System information (ESP32-specific)
    static uint32_t getFreeHeap() { return ESP.getFreeHeap(); }
    static uint32_t getHeapSize() { return ESP.getHeapSize(); }
    static uint32_t getCpuFreqMHz() { return ESP.getCpuFreqMHz(); }
    static uint32_t getMinFreeHeap() { return ESP.getMinFreeHeap(); }
    // HAL factory methods (ESP32-specific)
    static SDCardController *createSDCardController() { return new CrowPanelSDCardController(); }
};

#endif // PLATFORM_CROW_PANEL