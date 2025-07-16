#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "../hal/display/DisplayQueue.h"
#include <WiFi.h>
#include <ArduinoBLE.h>

/**
 * System monitor task - monitors system health, FreeRTOS tasks, and memory usage
 * Enhanced with comprehensive FreeRTOS monitoring capabilities
 */
class SystemMonitorTask : public SRTask {
public:
    SystemMonitorTask(uint32_t intervalMs = 10000)  // Every 10 seconds
        : SRTask("SysMonitor", 4096, tskIDLE_PRIORITY + 1, 0),  // Core 0
          _intervalMs(intervalMs) {}
    
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
    
protected:
    void run() override {
        LOG_INFO("SystemMonitorTask started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Log comprehensive system status (no display rendering needed)
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
        
        // Log basic system status
        LOG_PRINTF("System Status - Uptime: %ds, Heap: %d/%d bytes (%.1f%%), Min: %d bytes", 
                   uptime, freeHeap, totalHeap, 
                   (float)freeHeap / totalHeap * 100.0f, minFreeHeap);
        
        // Log queue status if available
        uint32_t itemCount, spacesAvailable;
        LogManager::getInstance().getStatus(itemCount, spacesAvailable);
        LOG_PRINTF("Log Queue - Items: %d, Available: %d", itemCount, spacesAvailable);
        
        // FreeRTOS Task Statistics
        logTaskStatistics();
        
        // CPU Usage Monitoring
        logCPUUsage();
        
        // Memory Fragmentation Analysis
        logMemoryFragmentation();
        
        // Power Consumption Monitoring
        logPowerConsumption();
    }
    
    void logTaskStatistics() {
        // Get number of tasks
        UBaseType_t taskCount = uxTaskGetNumberOfTasks();
        LOG_PRINTF("FreeRTOS Tasks: %d total", taskCount);
        
        // Get current task info
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        UBaseType_t currentPriority = uxTaskPriorityGet(currentTask);
        UBaseType_t currentCore = xPortGetCoreID();
        
        LOG_PRINTF("Current Task: %s (Priority: %d, Core: %d)", 
                   pcTaskGetName(currentTask), currentPriority, currentCore);
    }
    
    void logCPUUsage() {
        // Simple CPU usage estimation based on task count and priorities
        UBaseType_t taskCount = uxTaskGetNumberOfTasks();
        LOG_PRINTF("CPU Usage - Active Tasks: %d", taskCount);
        
        // Note: For more accurate CPU usage, you'd need to implement
        // a custom idle hook or use ESP32-specific APIs
    }
    
    void logMemoryFragmentation() {
        // Get memory fragmentation info
        size_t largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        size_t freeBlocks = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        
        LOG_PRINTF("Memory Fragmentation - Largest Block: %d bytes, Free: %d bytes", 
                   largestFreeBlock, freeBlocks);
        
        // Check for potential memory issues
        if (freeBlocks < 10000) {  // Less than 10KB free
            LOG_WARNF("Low memory warning - Only %d bytes free", freeBlocks);
        }
        
        if (largestFreeBlock < 1000) {  // Largest block less than 1KB
            LOG_WARNF("Memory fragmentation warning - Largest block only %d bytes", largestFreeBlock);
        }
    }
    
    void logPowerConsumption() {
        // Get CPU frequency (affects power consumption significantly)
        uint32_t cpuFreq = ESP.getCpuFreqMHz();
        
        // Get current CPU usage estimation
        UBaseType_t taskCount = uxTaskGetNumberOfTasks();
        
        // Get WiFi power state (if available)
        bool wifiEnabled = WiFi.status() != WL_DISCONNECTED;
        
        // Get BLE power state
        bool bleEnabled = BLE.connected() || BLE.advertise();
        
        // Calculate power consumption estimate
        // This is a rough estimation based on ESP32 power characteristics
        float estimatedPower = 0.0f;
        
        // Base power consumption
        estimatedPower += 50.0f;  // Base ESP32 power ~50mA
        
        // CPU frequency impact (higher freq = more power)
        estimatedPower += (cpuFreq - 80) * 0.5f;  // ~0.5mA per MHz above 80MHz
        
        // Task count impact (more tasks = more context switching = more power)
        if (taskCount > 5) {
            estimatedPower += (taskCount - 5) * 2.0f;  // ~2mA per additional task
        }
        
        // WiFi power (if enabled)
        if (wifiEnabled) {
            estimatedPower += 30.0f;  // WiFi adds ~30mA
        }
        
        // BLE power (if enabled)
        if (bleEnabled) {
            estimatedPower += 15.0f;  // BLE adds ~15mA
        }
        
        // Log power information
        LOG_PRINTF("Power Status - CPU: %dMHz, Tasks: %d, WiFi: %s, BLE: %s", 
                   cpuFreq, taskCount, wifiEnabled ? "ON" : "OFF", bleEnabled ? "ON" : "OFF");
        LOG_PRINTF("Power Estimate: %.1f mA (Base: 50mA + CPU: %.1fmA + Tasks: %.1fmA + Radio: %.1fmA)", 
                   estimatedPower, 
                   (cpuFreq - 80) * 0.5f,
                   (taskCount > 5) ? (taskCount - 5) * 2.0f : 0.0f,
                   (wifiEnabled ? 30.0f : 0.0f) + (bleEnabled ? 15.0f : 0.0f));
        
        // Power optimization suggestions
        if (estimatedPower > 150.0f) {
            LOG_WARNF("High power consumption detected: %.1f mA", estimatedPower);
            LOG_INFO("Power optimization suggestions:");
            LOG_INFO("- Reduce CPU frequency if possible");
            LOG_INFO("- Consolidate tasks to reduce context switching");
            LOG_INFO("- Disable unused radio features");
        }
        
        // Monitor for power spikes (sudden increases in task count)
        static uint32_t lastTaskCount = 0;
        if (taskCount > lastTaskCount + 2) {
            LOG_WARNF("Power spike detected: Task count increased from %d to %d", 
                     lastTaskCount, taskCount);
        }
        lastTaskCount = taskCount;
    }
    
    uint32_t _intervalMs;
}; 