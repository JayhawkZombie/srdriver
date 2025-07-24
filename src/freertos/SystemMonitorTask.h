#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "../hal/display/DisplayQueue.h"
#include "../hal/SSD_1306Component.h"
#include <WiFi.h>
#include <ArduinoBLE.h>
#include "../hal/temperature/DS18B20Component.h"
// Forward declaration
extern SSD1306_Display display;
extern DS18B20Component *g_temperatureSensor;


/**
 * System monitor task - monitors system health, FreeRTOS tasks, and memory usage
 * Now uses simple display ownership system to render system stats
 */
class SystemMonitorTask : public SRTask {
public:
    SystemMonitorTask(uint32_t intervalMs = 5000)  // Every 5 seconds
        : SRTask("SysMonitor", 4096, tskIDLE_PRIORITY + 1, 0),  // Core 0
          _intervalMs(intervalMs)
        , _displayUpdateInterval(1000)  // Display updates every 1 second
        , _lastDisplayUpdate(0) {}
    
    // Get detailed task information for external monitoring
    void logDetailedTaskInfo() {
        LOG_INFO("=== Detailed Task Information ===");
        
        // Get current task info
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        UBaseType_t currentPriority = uxTaskPriorityGet(currentTask);
        UBaseType_t currentCore = xPortGetCoreID();
        
        LOG_PRINTF("Current Task: %s (Priority: %d, Core: %d)", 
                   pcTaskGetName(currentTask), currentPriority, currentCore);
        
        // Get total task count
        UBaseType_t taskCount = uxTaskGetNumberOfTasks();
        LOG_PRINTF("Total FreeRTOS Tasks: %d", taskCount);
        
        // Get stack high water mark for current task
        uint32_t stackHighWaterMark = uxTaskGetStackHighWaterMark(currentTask);
        LOG_PRINTF("Current Task Stack High Water Mark: %d bytes", stackHighWaterMark);
        
        LOG_INFO("=== End Task Information ===");
    }
    
    // Power optimization methods
    void suggestPowerOptimizations() {
        uint32_t cpuFreq = ESP.getCpuFreqMHz();
        UBaseType_t taskCount = uxTaskGetNumberOfTasks();
        
        LOG_INFO("=== Power Optimization Suggestions ===");
        
        // CPU frequency suggestions
        if (cpuFreq > 160) {
            LOG_PRINTF("Consider reducing CPU frequency from %dMHz to 160MHz for power savings", cpuFreq);
        }
        
        // Task count suggestions
        if (taskCount > 8) {
            LOG_PRINTF("Consider consolidating tasks (current: %d) to reduce context switching", taskCount);
        }
        
        // WiFi suggestions
        if (WiFi.status() != WL_DISCONNECTED) {
            LOG_INFO("Consider disabling WiFi if not needed (saves ~30mA)");
        }
        
        // BLE suggestions
        if (BLE.connected() || BLE.advertise()) {
            LOG_INFO("BLE is active - consider reducing advertising interval if possible");
        }
        
        LOG_INFO("=== End Power Suggestions ===");
    }

    void logTemperature()
    {
        if (g_temperatureSensor)
        {
            g_temperatureSensor->update();
            const auto tempC = g_temperatureSensor->getTemperatureC();
            const auto tempF = g_temperatureSensor->getTemperatureF();
            LOG_PRINTF("Temperature: %.1f°C / %.1f°F", tempC, tempF);
        }
    }
    
    // Get power efficiency score (0-100, higher is better)
    uint8_t getPowerEfficiencyScore() {
        uint32_t cpuFreq = ESP.getCpuFreqMHz();
        UBaseType_t taskCount = uxTaskGetNumberOfTasks();
        bool wifiEnabled = WiFi.status() != WL_DISCONNECTED;
        bool bleEnabled = BLE.connected() || BLE.advertise();
        
        uint8_t score = 100;
        
        // Deduct points for high CPU frequency
        if (cpuFreq > 240) score -= 20;
        else if (cpuFreq > 160) score -= 10;
        
        // Deduct points for too many tasks
        if (taskCount > 10) score -= 20;
        else if (taskCount > 6) score -= 10;
        
        // Deduct points for enabled radios
        if (wifiEnabled) score -= 15;
        if (bleEnabled) score -= 10;
        
        return score;
    }
    
    // Static render function for display ownership
    static void renderSystemStats(SSD1306_Display& display);
    
protected:
    void run() override {
        LOG_INFO("SystemMonitorTask started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Check if it's time for display update
            uint32_t now = millis();
            if (now - _lastDisplayUpdate >= _displayUpdateInterval) {
                updateDisplay();
                _lastDisplayUpdate = now;
            }

            logTemperature();
            
            // Log comprehensive system status (less frequent)
            logSystemStatus();
            
            // Sleep until next cycle
            SRTask::sleepUntil(&lastWakeTime, _intervalMs);
        }
    }
    
private:
    void logSystemStatus();
    void logTaskStatistics();
    void logCPUUsage();
    void logMemoryFragmentation();
    void logPowerConsumption();
    void updateDisplay();
    
    uint32_t _intervalMs;
    uint32_t _displayUpdateInterval;
    uint32_t _lastDisplayUpdate;
}; 