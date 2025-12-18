#pragma once

#include "SRTask.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "Esp.h"
#include <Arduino.h>
#include "PlatformConfig.h"
#include "LogManager.h"

#if SUPPORTS_POWER_SENSORS
#include "hal/power/ACS712CurrentSensor.h"
#include "hal/power/ACS712VoltageSensor.h"
#endif

/**
 * SystemStats - Structure holding all collected system statistics
 * This is the data model that renderers can query
 */
struct SystemStats {
    // Uptime
    uint32_t uptimeSeconds;
    uint32_t uptimeDays;
    uint32_t uptimeHours;
    uint32_t uptimeMinutes;
    
    // Memory
    uint32_t freeHeap;
    uint32_t totalHeap;
    uint32_t minFreeHeap;
    uint8_t heapUsagePercent;
    
    // Tasks
    UBaseType_t taskCount;
    
    // CPU
    uint32_t cpuFreqMHz;
    
    // Temperature (if available)
    float temperatureC;
    float temperatureF;
    bool temperatureAvailable;
    
    // Power (if available)
    float current_mA;
    float voltage_V;
    float power_W;
    bool powerAvailable;
    
    // Timestamp
    uint32_t lastUpdateTime;  // millis() when stats were last updated
};

/**
 * SystemMonitorTask - Collects system statistics without rendering
 *
 * This task periodically collects system information and stores it in a
 * thread-safe SystemStats structure. Renderers (like DisplayTask or LVGL UI)
 * can query these stats and render them in their own format.
 */
class SystemMonitorTask : public SRTask {
public:
    SystemMonitorTask(uint32_t updateIntervalMs = 1000)
        : SRTask("SysMonitor", 4096, tskIDLE_PRIORITY + 1, 0),  // Core 0
        _updateIntervalMs(updateIntervalMs),
        _statsMutex(xSemaphoreCreateMutex())
    {
        // Initialize stats to zero
        memset(&_stats, 0, sizeof(_stats));
    }
    
    ~SystemMonitorTask()
    {
        if (_statsMutex)
        {
            vSemaphoreDelete(_statsMutex);
        }
#if SUPPORTS_POWER_SENSORS
        if (_currentSensor) {
            delete _currentSensor;
            _currentSensor = nullptr;
        }
        if (_voltageSensor) {
            delete _voltageSensor;
            _voltageSensor = nullptr;
        }
#endif
    }
    
    /**
     * Get current system stats (thread-safe)
     * @return Copy of current SystemStats
     */
    SystemStats getStats() const
    {
        SystemStats stats;
        if (_statsMutex && xSemaphoreTake(_statsMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            stats = _stats;
            xSemaphoreGive(_statsMutex);
        }
        return stats;
    }
    
    /**
     * Individual getters for convenience (thread-safe)
     */
    uint32_t getUptimeSeconds() const
    {
        SystemStats stats = getStats();
        return stats.uptimeSeconds;
    }
    
    UBaseType_t getTaskCount() const
    {
        SystemStats stats = getStats();
        return stats.taskCount;
    }
    
    uint32_t getFreeHeap() const
    {
        SystemStats stats = getStats();
        return stats.freeHeap;
    }
    
    uint8_t getHeapUsagePercent() const
    {
        SystemStats stats = getStats();
        return stats.heapUsagePercent;
    }
    
    uint32_t getCpuFreqMHz() const
    {
        SystemStats stats = getStats();
        return stats.cpuFreqMHz;
    }
    
    float getTemperatureF() const
    {
        SystemStats stats = getStats();
        return stats.temperatureF;
    }

#if SUPPORTS_POWER_SENSORS
    /**
     * Initialize power sensors (call before starting the task)
     * @param currentPin Analog pin for current sensor
     * @param voltagePin Analog pin for voltage sensor
     * @param variant ACS712 variant (default: ACS712_30A)
     * @param supplyVoltage Supply voltage (default: 5.0V)
     * @param adcReference ADC reference voltage (default: 3.3V)
     */
    void initializePowerSensors(uint8_t currentPin = A2, 
                                uint8_t voltagePin = A3,
                                ACS712Variant variant = ACS712_30A,
                                float supplyVoltage = 5.0f,
                                float adcReference = 3.3f)
    {
        if (_currentSensor || _voltageSensor) {
            LOG_WARN_COMPONENT("SystemMonitor", "Power sensors already initialized");
            return;
        }

        LOG_INFO_COMPONENT("SystemMonitor", "Initializing power sensors...");
        _currentSensor = new ACS712CurrentSensor(currentPin, variant, supplyVoltage, adcReference);
        _currentSensor->begin();
        _currentSensor->setPolarityCorrection(false);

        _voltageSensor = new ACS712VoltageSensor(voltagePin, adcReference, supplyVoltage);
        _voltageSensor->begin();

        LOG_INFO_COMPONENT("SystemMonitor", "Power sensors initialized successfully");
    }

    /**
     * Force recalibration of power sensors
     * @return true if recalibration was successful
     */
    bool forceRecalibratePowerSensors()
    {
        if (!_currentSensor) {
            LOG_WARN_COMPONENT("SystemMonitor", "Power sensors not initialized");
            return false;
        }

        LOG_INFO_COMPONENT("SystemMonitor", "Forcing power sensor recalibration...");
        bool success = _currentSensor->forceRecalibration();
        if (success) {
            LOG_INFO_COMPONENT("SystemMonitor", "Power sensor recalibration successful");
        } else {
            LOG_WARN_COMPONENT("SystemMonitor", "Power sensor recalibration failed");
        }
        return success;
    }
#endif

protected:
    void run() override
    {
        LOG_INFO_COMPONENT("SystemMonitor", "SystemMonitorTask started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true)
        {
            // Collect system statistics
            updateStats();
            
            // Sleep until next update
            SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
        }
    }
    
private:
    /**
     * Update system statistics (called from run loop)
     */
    void updateStats()
    {
        if (!_statsMutex) return;
        
        if (xSemaphoreTake(_statsMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            uint32_t now = millis();
            
            // Uptime
            _stats.uptimeSeconds = now / 1000;
            _stats.uptimeDays = _stats.uptimeSeconds / 86400;
            _stats.uptimeHours = (_stats.uptimeSeconds % 86400) / 3600;
            _stats.uptimeMinutes = (_stats.uptimeSeconds % 3600) / 60;
            
            // Memory (using ESP32 APIs via Esp.h)
            _stats.freeHeap = ESP.getFreeHeap();
            _stats.totalHeap = ESP.getHeapSize();
            _stats.minFreeHeap = ESP.getMinFreeHeap();
            _stats.cpuFreqMHz = ESP.getCpuFreqMHz();
            
            // Calculate heap usage percentage
            if (_stats.totalHeap > 0)
            {
                _stats.heapUsagePercent = 100 - ((_stats.freeHeap * 100) / _stats.totalHeap);
            }
            else
            {
                _stats.heapUsagePercent = 0;
            }
            
            // Tasks
            _stats.taskCount = uxTaskGetNumberOfTasks();
            
            // Temperature (not available on CrowPanel by default, but structure ready)
            _stats.temperatureC = 0.0f;
            _stats.temperatureF = 0.0f;
            _stats.temperatureAvailable = false;
            
            // Power sensors (if available)
#if SUPPORTS_POWER_SENSORS
            if (_currentSensor && _voltageSensor) {
                _stats.current_mA = _currentSensor->readCurrentDCFiltered_mA();
                _stats.voltage_V = _voltageSensor->readVoltageDCFiltered_V();
                _stats.power_W = (_stats.current_mA / 1000.0f) * _stats.voltage_V;
                _stats.powerAvailable = true;
            } else {
                _stats.current_mA = 0.0f;
                _stats.voltage_V = 0.0f;
                _stats.power_W = 0.0f;
                _stats.powerAvailable = false;
            }
#else
            _stats.current_mA = 0.0f;
            _stats.voltage_V = 0.0f;
            _stats.power_W = 0.0f;
            _stats.powerAvailable = false;
#endif
            
            // Timestamp
            _stats.lastUpdateTime = now;
            
            xSemaphoreGive(_statsMutex);
        }
    }
    
    uint32_t _updateIntervalMs;
    SystemStats _stats;
    SemaphoreHandle_t _statsMutex;

#if SUPPORTS_POWER_SENSORS
    ACS712CurrentSensor* _currentSensor = nullptr;
    ACS712VoltageSensor* _voltageSensor = nullptr;
#endif
};
