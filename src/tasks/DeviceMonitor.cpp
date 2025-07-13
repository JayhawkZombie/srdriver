#include "DeviceMonitor.h"
#include "utility/LogManager.h"
#include <SD.h>
#include <esp_system.h>

// External reference to global SD card availability flag
extern bool g_sdCardAvailable;

DeviceMonitor::DeviceMonitor() 
    : active(false)
    , lastMonitorTime(0)
    , monitorInterval(30000) // Default: 30 seconds
    , lastTaskTime(0)
    , taskCount(0)
    , sdCardAvailable(false) {
}

void DeviceMonitor::begin() {
    active = true;
    lastMonitorTime = millis();
    lastTaskTime = millis();
    taskCount = 0;
    
    // Use global SD card availability flag
    sdCardAvailable = g_sdCardAvailable;
    
    LOG_INFO("Device monitor started - interval: " + String(monitorInterval) + "ms");
    if (sdCardAvailable) {
        LOG_INFO("SD card detected and accessible");
    } else {
        LOG_INFO("No SD card detected - logging to memory only");
    }
}

void DeviceMonitor::update() {
    if (!active) return;
    
    unsigned long now = millis();
    taskCount++;
    
    // Check if it's time to log system stats
    if (now - lastMonitorTime >= monitorInterval) {
        unsigned long executionStart = micros(); // Start timing only for the health check
        SystemStats stats = getCurrentStats();
        
        // Calculate actual execution time for this health report
        unsigned long executionTime = micros() - executionStart;
        stats.taskExecutionTime = executionTime;
        
        // Log system health
        LOG_INFO("=== System Health Report ===");
        LOG_INFO("Uptime: " + String(stats.uptime / 1000) + "s");
        LOG_INFO("Free heap: " + String(stats.freeHeap) + "/" + String(stats.totalHeap) + " bytes (" + 
                String((stats.freeHeap * 100) / stats.totalHeap) + "%)");
        LOG_INFO("CPU usage: " + String(stats.cpuUsage, 1) + "%");
        LOG_INFO("Task scheduler interval: " + String(stats.taskSchedulerInterval) + "ms");
        LOG_INFO("Task execution time: " + String(stats.taskExecutionTime) + "μs");
        LOG_INFO("Log queue size: " + String(stats.logQueueSize));
        
        if (sdCardAvailable) {
            LOG_INFO("SD card free space: " + String(stats.sdCardFreeSpace / 1024) + " KB [placeholder, not implemented]");
        } else {
            LOG_INFO("SD card: Not available");
        }
        
        LOG_INFO("Temperature: " + String(stats.temperature, 1) + "°C [placeholder, not implemented]");
        
        // Check for warnings (only for real values)
        if (stats.freeHeap < 10000) {
            LOG_WARN("Low memory warning: " + String(stats.freeHeap) + " bytes free");
        }
        
        if (stats.cpuUsage > 80.0) {
            LOG_WARN("High CPU usage: " + String(stats.cpuUsage, 1) + "%");
        }
        
        if (stats.taskExecutionTime > 1000) { // More than 1ms execution time
            LOG_WARN("Slow task execution: " + String(stats.taskExecutionTime) + "μs");
        }
        
        if (stats.logQueueSize > 50) {
            LOG_WARN("Log queue backing up: " + String(stats.logQueueSize) + " entries");
        }
        
        // Skip SD card space warning since it's a placeholder
        // if (stats.sdCardFreeSpace < 1024 * 1024) { // Less than 1MB
        //     LOG_WARN("Low SD card space: " + String(stats.sdCardFreeSpace / 1024) + " KB");
        // }
        
        LOG_INFO("=== End Health Report ===");
        
        lastMonitorTime = now;
        lastTaskTime = now;
        taskCount = 0;
    }
}

DeviceMonitor::SystemStats DeviceMonitor::getCurrentStats() const {
    SystemStats stats;
    
    stats.freeHeap = ESP.getFreeHeap();
    stats.totalHeap = ESP.getHeapSize();
    stats.uptime = millis();
    stats.cpuUsage = calculateCpuUsage();
    stats.taskSchedulerInterval = (millis() - lastTaskTime) / (taskCount > 0 ? taskCount : 1);
    stats.taskExecutionTime = 0; // Will be set by the calling update() method
    stats.sdCardFreeSpace = getSDCardFreeSpace();
    stats.temperature = getTemperature();
    stats.logQueueSize = LogManager::getInstance().getQueueSize();
    stats.lastResetReason = getResetReason();
    
    return stats;
}

float DeviceMonitor::calculateCpuUsage() const {
    // Simple CPU usage calculation based on task execution time
    // This is a rough estimate - more accurate methods would require hardware timers
    unsigned long totalTime = millis() - lastTaskTime;
    unsigned long busyTime = taskCount * 2; // Estimate 2ms per task
    return (totalTime > 0) ? (float(busyTime) * 100.0 / totalTime) : 0.0;
}

unsigned long DeviceMonitor::getSDCardFreeSpace() const {
    if (!sdCardAvailable) {
        return 0; // No SD card available
    }
    
    // Note: Arduino SD library doesn't provide free space directly
    // This would need to be implemented with SdFat library or similar
    // For now, return a placeholder
    return 0; // TODO: Implement actual SD card space checking
}

float DeviceMonitor::getTemperature() const {
    // ESP32 internal temperature sensor (if available)
    // Note: This might not be available on all ESP32 variants
    return 0.0; // TODO: Implement temperature reading if available
}

unsigned long DeviceMonitor::getResetReason() const {
    // Get the last reset reason from ESP32
    esp_reset_reason_t reason = esp_reset_reason();
    return (unsigned long)reason;
} 