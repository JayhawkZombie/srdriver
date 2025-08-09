#include "SystemMonitorTask.h"
#include "DisplayTask.h"
#include "PlatformConfig.h"
#include "hal/PlatformFactory.h"
#include "hal/SDCardAPI.h"
#include "utility/TimeUtils.hpp"
#include "hal/display/DisplayQueue.h"
#include "PatternManager.h"

// Static render function for display ownership
void SystemMonitorTask::renderSystemStats(SSD1306_Display& display) {
    // Safety check: Don't render if system isn't ready
    if (millis() < SecondsToMs(5)) {
        return;  // Too early in boot
    }
    
    // Get system stats with error checking using platform abstractions
    uint32_t uptime = millis() / 1000;  // Convert to seconds
    uint32_t freeHeap = PlatformFactory::getFreeHeap();
    uint32_t totalHeap = PlatformFactory::getHeapSize();
    
    // Safety check: Don't divide by zero
    if (totalHeap == 0) {
        totalHeap = 1;  // Prevent division by zero
    }
    
    uint8_t heapUsagePercent = (100 - (freeHeap * 100 / totalHeap));
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    uint32_t cpuFreq = PlatformFactory::getCpuFreqMHz();
    
    // Set text properties
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(15, "System Status", 1);  // Moved up from 20
    
    // Uptime
    char uptimeText[32];
    snprintf(uptimeText, sizeof(uptimeText), "Uptime: %ds", uptime);
    display.printAt(2, 25, uptimeText, 1);  // Moved up from 30
    
    // Heap usage
    char heapText[32];
    snprintf(heapText, sizeof(heapText), "Heap: %d%% (%dKB)", heapUsagePercent, freeHeap/1024);
    display.printAt(2, 35, heapText, 1);  // Moved up from 40
    
    // Draw a simple progress bar for heap usage (on separate line)
    int barWidth = 100;
    int barHeight = 4;
    int barX = 14;
    int barY = 45;  // Moved up from 50
    
    // Draw background
    display.drawRect(barX, barY, barWidth, barHeight, COLOR_WHITE);
    
    // Draw fill
    int fillWidth = (barWidth - 2) * heapUsagePercent / 100;
    if (fillWidth > 0) {
        display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, COLOR_WHITE);
    }
    
    // Task count and CPU
    char taskText[32];
    float temperature = g_temperatureSensor->getTemperatureF();
    snprintf(taskText, sizeof(taskText), "Ts: %d %dMH %.1f F", taskCount, cpuFreq, temperature);
    display.printAt(2, 55, taskText, 1);  // Moved up from 60
}

void SystemMonitorTask::updateDisplay() {
    static uint32_t ownershipStartTime = 0;
    static bool ownershipActive = false;
    
    // Safety check: Don't try to access display too early in boot
    static uint32_t bootTime = millis();
    if (bootTime < SecondsToMs(1)) {  // Wait 1 second after boot before trying display
        return;
    }
    
    // Try to request display ownership
    if (!ownershipActive && DisplayTask::requestOwnership("SystemMonitor", renderSystemStats)) {
        ownershipActive = true;
        ownershipStartTime = millis();
        LOG_DEBUGF("SystemMonitorTask started display ownership");
    } else if (ownershipActive) {
        // Check if it's time to release ownership
        if (millis() - ownershipStartTime >= 2000) {
            // Release ownership after 2 seconds
            if (DisplayTask::releaseOwnership("SystemMonitor")) {
                LOG_DEBUGF("SystemMonitorTask released display ownership");
            }
            ownershipActive = false;
            ownershipStartTime = 0;
        }
    }
}

void SystemMonitorTask::logSystemStatus() {
    // Get heap info
    uint32_t freeHeap = PlatformFactory::getFreeHeap();
    uint32_t totalHeap = PlatformFactory::getHeapSize();
    uint32_t minFreeHeap = PlatformFactory::getMinFreeHeap();
    
    // Get uptime
    uint32_t uptime = MsToSeconds(millis());
    
    // Log basic system status
    LOG_PRINTF("System Status - Uptime: %ds, Heap: %d/%d bytes (%.1f%%), Min: %d bytes", 
               uptime, freeHeap, totalHeap, 
               (float)freeHeap / totalHeap * 100.0f, minFreeHeap);
    
    // FreeRTOS Task Statistics
    logTaskStatistics();
    
    // CPU Usage Monitoring
    logCPUUsage();
    
    // Memory Fragmentation Analysis
    logMemoryFragmentation();
    
    // Power Consumption Monitoring
    // logPowerConsumption();
}

void SystemMonitorTask::logTaskStatistics() {
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

void SystemMonitorTask::logCPUUsage() {
    // Simple CPU usage estimation based on task count and priorities
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    LOG_PRINTF("CPU Usage - Active Tasks: %d", taskCount);
    
    // Note: For more accurate CPU usage, you'd need to implement
    // a custom idle hook or use ESP32-specific APIs
}

void SystemMonitorTask::logMemoryFragmentation() {
    // Get memory fragmentation info (platform-specific)
    size_t largestFreeBlock = 0;
    size_t freeBlocks = 0;
    
    #if SUPPORTS_ESP32_APIS
    largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    freeBlocks = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    #else
    // For non-ESP32 platforms, use basic heap info
    freeBlocks = PlatformFactory::getFreeHeap();
    largestFreeBlock = freeBlocks; // Rough estimate for non-ESP32
    #endif
    
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

void SystemMonitorTask::logPowerConsumption() {
    // Get CPU frequency (affects power consumption significantly)
    uint32_t cpuFreq = PlatformFactory::getCpuFreqMHz();
    
    // Get current CPU usage estimation
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    
    // Get WiFi power state (if available)
    bool wifiEnabled = false;
    #if SUPPORTS_WIFI
    wifiEnabled = WiFi.status() != WL_DISCONNECTED;
    #endif
    
    // Get BLE power state
    bool bleEnabled = false;
    #if SUPPORTS_BLE
    bleEnabled = BLE.connected() || BLE.advertise();
    #endif
    
    // Calculate power consumption estimate
    // This is a rough estimation based on platform power characteristics
    float estimatedPower = 0.0f;
    
    // Base power consumption (platform-specific)
    #if PLATFORM_ESP32_S3
    estimatedPower += 50.0f;  // Base ESP32-S3 power ~50mA
    #elif PLATFORM_RP2040
    estimatedPower += 30.0f;  // Base RP2040 power ~30mA
    #else
    estimatedPower += 40.0f;  // Default estimate
    #endif
    
    // CPU frequency impact (higher freq = more power)
    #if PLATFORM_ESP32_S3
    estimatedPower += (cpuFreq - 80) * 0.5f;  // ~0.5mA per MHz above 80MHz
    #elif PLATFORM_RP2040
    estimatedPower += (cpuFreq - 48) * 0.3f;  // ~0.3mA per MHz above 48MHz
    #else
    estimatedPower += (cpuFreq - 80) * 0.4f;  // Default estimate
    #endif
    
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
    LOG_PRINTF("Power Estimate: %.1f mA (Base: %.1fmA + CPU: %.1fmA + Tasks: %.1fmA + Radio: %.1fmA)", 
               estimatedPower, 
               #if PLATFORM_ESP32_S3
               50.0f,
               (cpuFreq - 80) * 0.5f,
               #elif PLATFORM_RP2040
               30.0f,
               (cpuFreq - 48) * 0.3f,
               #else
               40.0f,
               (cpuFreq - 80) * 0.4f,
               #endif
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

void SystemMonitorTask::monitorSDCard() {
    static unsigned long lastMonitorTime = 0;
    // Monitoring the SD card every 10 minutes, and also right at boot
    const auto timeSince = millis() - lastMonitorTime;
    if (timeSince < MinutesToMs(10) && millis() > SecondsToMs(5)) {
        return;
    }
    lastMonitorTime = millis();
    LOG_INFO("Monitoring SD card");

    // Get size of /logs/srdriver.log
    const auto logSize = SDCardAPI::getInstance().getFileSize("/logs/srdriver.log");
    const auto logSizeMB = logSize / 1024 / 1024;
    LOG_DEBUGF("Log size: %d bytes (%dMB)", logSize, logSizeMB);
    
    // Log SD card info
    LOG_PRINTF("SD Card - Log size: %dMB", logSizeMB);
    if (logSizeMB > 100) {
        LOG_DEBUGF("Log size is greater than 100MB (%dMB), deleting old logs", logSizeMB);
        g_sdCardController->remove("/logs/srdriver_old.log");
        g_sdCardController->rename("/logs/srdriver.log", "/logs/srdriver_old.log");
        g_sdCardController->writeFile("/logs/srdriver.log", "<log rotated>");
        LOG_DEBUG("Log rotated");
        DisplayQueue::getInstance().safeRequestBannerMessage("SysMon", "Logs rotated (size)");
    }
}
