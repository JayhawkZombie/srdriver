#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "../hal/display/DisplayQueue.h"
#include "../hal/display/SSD_1306Component.h"
#include <WiFi.h>
#include <ArduinoBLE.h>
#include "../hal/temperature/DS18B20Component.h"
#include "../PatternManager.h"
#include "../GlobalState.h"
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

    // Thermal brightness curve mapping - uses same exponential curve as potentiometer
    float getThermalBrightnessCurve(float currentBrightnessNormalized)
    {
        // Use the same exponential curve as potentiometer: (exp(value) - 1) / (exp(1) - 1)
        const auto numerator = exp(currentBrightnessNormalized) - 1.0f;
        const auto denominator = exp(1.0f) - 1.0f;
        return numerator / denominator;
    }

    void logTemperature()
    {
        static bool isAlertActive = false;
        static int lastBrightness = 0;
        if (g_temperatureSensor)
        {
            g_temperatureSensor->update();
            const auto tempC = g_temperatureSensor->getTemperatureC();
            const auto tempF = g_temperatureSensor->getTemperatureF();
            LOG_PRINTF("Temperature: %.1f°C / %.1f°F", tempC, tempF);
            if (tempF > 110.0f) {
                LOG_WARNF("High temperature detected (%.1fF), setting alert wave player", tempF);
                if (!isAlertActive) {
                    SetAlertWavePlayer("high_temperature");
                    // Store current brightness and reduce it using exponential curve mapping
                    lastBrightness = deviceState.brightness;
                    float currentBrightnessNormalized = lastBrightness / 255.0f;  // Convert to 0.0-1.0
                    float thermalCurveValue = getThermalBrightnessCurve(currentBrightnessNormalized);
                    int reducedBrightness = 10;// max(25, (int)(thermalCurveValue * 255.0f * 0.3f));  // Apply 30% of curve value
                    LOG_INFOF("Current brightness: %d, curve value: %.3f, reducing to: %d", lastBrightness, thermalCurveValue, reducedBrightness);
                    UpdateBrightnessInt(reducedBrightness);
                    LOG_INFOF("Reduced brightness from %d to %d using exponential curve mapping", lastBrightness, reducedBrightness);
                    isAlertActive = true;
                }
            } else if (tempF <= 100.0f) {  // Stop alert when temp drops 5°F below threshold
                LOG_INFOF("Temperature normalized (%.1fF), stopping alert wave player", tempF);
                StopAlertWavePlayer("temperature_normalized");
                // Restore original brightness
                if (isAlertActive) {
                    LOG_INFOF("Restoring brightness from %d to %d", deviceState.brightness, lastBrightness);
                    UpdateBrightnessInt(lastBrightness);
                    LOG_INFOF("Restored brightness to %d", lastBrightness);
                }
                isAlertActive = false;
            }
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

    void monitorSDCard();
    
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

            monitorSDCard();
            
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