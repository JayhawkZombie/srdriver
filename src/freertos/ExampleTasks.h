#pragma once

#include "SRTask.h"
#include "LogManager.h"

/**
 * Simple LED blink task - demonstrates basic task usage
 */
class LEDBlinkTask : public SRTask {
public:
    LEDBlinkTask(uint8_t pin = LED_BUILTIN, uint32_t intervalMs = 1000)
        : SRTask("LEDBlink", 2048, tskIDLE_PRIORITY + 1, 1),  // Pin to core 1
          _pin(pin), _intervalMs(intervalMs) {}
    
protected:
    void run() override {
        pinMode(_pin, OUTPUT);
        LOG_INFO("LEDBlinkTask started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            digitalWrite(_pin, HIGH);
            LOG_DEBUG("LED ON");
            
            SRTask::sleepUntil(&lastWakeTime, _intervalMs / 2);
            
            digitalWrite(_pin, LOW);
            LOG_DEBUG("LED OFF");
            
            SRTask::sleepUntil(&lastWakeTime, _intervalMs / 2);
        }
    }
    
private:
    uint8_t _pin;
    uint32_t _intervalMs;
};

/**
 * System monitor task - demonstrates periodic logging and system status
 */
class SystemMonitorTask : public SRTask {
public:
    SystemMonitorTask(uint32_t intervalMs = 10000)  // Every 10 seconds
        : SRTask("SysMonitor", 4096, tskIDLE_PRIORITY + 1, 0),  // Core 0
          _intervalMs(intervalMs) {}
    
protected:
    void run() override {
        LOG_INFO("SystemMonitorTask started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Log system status
            logSystemStatus();
            
            // Sleep until next cycle
            SRTask::sleepUntil(&lastWakeTime, _intervalMs);
        }
    }
    
private:
    void logSystemStatus() {
        // Get heap info
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t totalHeap = ESP.getHeapSize();
        uint32_t minFreeHeap = ESP.getMinFreeHeap();
        
        // Get uptime
        uint32_t uptime = millis() / 1000;  // Convert to seconds
        
        // Log status
        LOG_PRINTF("System Status - Uptime: %ds, Heap: %d/%d bytes (%.1f%%), Min: %d bytes", 
                   uptime, freeHeap, totalHeap, 
                   (float)freeHeap / totalHeap * 100.0f, minFreeHeap);
        
        // Log queue status if available
        uint32_t itemCount, spacesAvailable;
        LogManager::getInstance().getStatus(itemCount, spacesAvailable);
        LOG_PRINTF("Log Queue - Items: %d, Available: %d", itemCount, spacesAvailable);
    }
    
    uint32_t _intervalMs;
}; 