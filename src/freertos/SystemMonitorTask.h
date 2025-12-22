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

#if SUPPORTS_TEMPERATURE_SENSOR
#include "hal/temperature/DS18B20Component.h"
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
        // SystemStats stats;
        // if (_statsMutex && xSemaphoreTake(_statsMutex, pdMS_TO_TICKS(400)) == pdTRUE)
        // {
        //     // stats = _stats;
        //     // _lastStats = _stats;
        //     xSemaphoreGive(_statsMutex);
        // }
        return _lastStats;
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

            // Every 5 seconds, log the stats
            static uint32_t lastLogTime = 0;
            if (millis() - lastLogTime > 5000) {
                /*
                    uint32_t uptimeSeconds;
                    uint32_t uptimeDays;
                    uint32_t uptimeHours;
                    uint32_t uptimeMinutes;
                */
                LOG_DEBUGF_COMPONENT("SystemMonitor", "System uptime: %d days, %d hours, %d minutes, %d seconds", 
                                    _stats.uptimeDays, _stats.uptimeHours, _stats.uptimeMinutes, _stats.uptimeSeconds);
                LOG_DEBUGF_COMPONENT("SystemMonitor", "System free heap: %d", _stats.freeHeap);
                LOG_DEBUGF_COMPONENT("SystemMonitor", "System total heap: %d", _stats.totalHeap);
                LOG_DEBUGF_COMPONENT("SystemMonitor", "System min free heap: %d", _stats.minFreeHeap);
                LOG_DEBUGF_COMPONENT("SystemMonitor", "System cpu frequency: %d MHz", _stats.cpuFreqMHz);
                LOG_DEBUGF_COMPONENT("SystemMonitor", "System temperature: %f C, %f F", _stats.temperatureC, _stats.temperatureF);
                lastLogTime = millis();
            }
            
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
        
        // Temp var to hold new stats while collecting
        // This is to help prevent long locks when updating
        // internal _state, so we only lock when re-assigning
        // it to the newly-collecting stats
        SystemStats tempStats;

        // if (xSemaphoreTake(_statsMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        // {
            uint32_t now = millis();
            
            // Uptime
            tempStats.uptimeSeconds = now / 1000;
            tempStats.uptimeDays = tempStats.uptimeSeconds / 86400;
            tempStats.uptimeHours = (tempStats.uptimeSeconds % 86400) / 3600;
            tempStats.uptimeMinutes = (tempStats.uptimeSeconds % 3600) / 60;
            
            // Memory (using ESP32 APIs via Esp.h)
            tempStats.freeHeap = ESP.getFreeHeap();
            tempStats.totalHeap = ESP.getHeapSize();
            tempStats.minFreeHeap = ESP.getMinFreeHeap();
            tempStats.cpuFreqMHz = ESP.getCpuFreqMHz();
            
            // Calculate heap usage percentage
            if (tempStats.totalHeap > 0)
            {
                tempStats.heapUsagePercent = 100 - ((tempStats.freeHeap * 100) / tempStats.totalHeap);
            }
            else
            {
                tempStats.heapUsagePercent = 0;
            }
            
            // Tasks
            tempStats.taskCount = uxTaskGetNumberOfTasks();
            
            // Temperature sensor (if available)
#if SUPPORTS_TEMPERATURE_SENSOR
            extern DS18B20Component* g_temperatureSensor;
            if (g_temperatureSensor) {
                g_temperatureSensor->update();
                tempStats.temperatureC = g_temperatureSensor->getTemperatureC();
                tempStats.temperatureF = g_temperatureSensor->getTemperatureF();
                tempStats.temperatureAvailable = true;
            } else {
                tempStats.temperatureC = 0.0f;
                tempStats.temperatureF = 0.0f;
                tempStats.temperatureAvailable = false;
            }
#else
            tempStats.temperatureC = 0.0f;
            tempStats.temperatureF = 0.0f;
            tempStats.temperatureAvailable = false;
#endif
            
            // Power sensors (if available)
#if SUPPORTS_POWER_SENSORS
            if (_currentSensor && _voltageSensor) {
                tempStats.current_mA = _currentSensor->readCurrentDCFiltered_mA();
                tempStats.voltage_V = _voltageSensor->readVoltageDCFiltered_V();
                tempStats.power_W = (tempStats.current_mA / 1000.0f) * tempStats.voltage_V;
                tempStats.powerAvailable = true;
            } else {
                tempStats.current_mA = 0.0f;
                tempStats.voltage_V = 0.0f;
                tempStats.power_W = 0.0f;
                tempStats.powerAvailable = false;
            }
#else
            tempStats.current_mA = 0.0f;
            tempStats.voltage_V = 0.0f;
            tempStats.power_W = 0.0f;
            tempStats.powerAvailable = false;
#endif
            
            // Timestamp
            tempStats.lastUpdateTime = now;

            // Take the mutes and update it, assign both _stats and _lastStats
            // so that we can now return _lastStats with these updated stats,
            // even if the mutex is locked the next time we want to get the stats
            // we aren't doing any locking
            if (xSemaphoreTake(_statsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                _stats = tempStats;
                _lastStats = _stats;

                xSemaphoreGive(_statsMutex);
            }

            // _lastStats = _stats;
            // xSemaphoreGive(_statsMutex);
        // } // if (xSemaphoreTake(_statsMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    }
    
    uint32_t _updateIntervalMs;
    SystemStats _stats;
    SystemStats _lastStats;
    SemaphoreHandle_t _statsMutex;

#if SUPPORTS_POWER_SENSORS
    ACS712CurrentSensor* _currentSensor = nullptr;
    ACS712VoltageSensor* _voltageSensor = nullptr;
#endif
};
