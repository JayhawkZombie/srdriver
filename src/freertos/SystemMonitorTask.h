#pragma once

#include "SRTask.h"
#include "LogManager.h"

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
    
protected:
    void run() override {
        LOG_INFO("SystemMonitorTask started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Log comprehensive system status
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
    
    uint32_t _intervalMs;
}; 