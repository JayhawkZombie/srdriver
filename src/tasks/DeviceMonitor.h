#pragma once
#include <Arduino.h>

class DeviceMonitor {
public:
    DeviceMonitor();
    
    // Initialize the monitor
    void begin();
    
    // Main update method called by task scheduler
    void update();
    
    // Check if monitoring is active
    bool isActive() const { return active; }
    
    // Set monitoring interval (in milliseconds)
    void setInterval(unsigned long interval) { monitorInterval = interval; }
    
    // Get current system stats
    struct SystemStats {
        unsigned long freeHeap;
        unsigned long totalHeap;
        unsigned long uptime;
        float cpuUsage;
        unsigned long taskSchedulerInterval;  // Renamed from taskExecutionTime
        unsigned long taskExecutionTime;      // New: actual time spent in code
        unsigned long sdCardFreeSpace;
        float temperature;
        unsigned long logQueueSize;
        unsigned long lastResetReason;
    };
    
    SystemStats getCurrentStats() const;

private:
    bool active;
    unsigned long lastMonitorTime;
    unsigned long monitorInterval;
    unsigned long lastTaskTime;
    unsigned long taskCount;
    bool sdCardAvailable;  // Track if SD card is available
    
    // Calculate CPU usage
    float calculateCpuUsage() const;
    
    // Get SD card free space
    unsigned long getSDCardFreeSpace() const;
    
    // Get system temperature
    float getTemperature() const;
    
    // Get reset reason
    unsigned long getResetReason() const;
}; 