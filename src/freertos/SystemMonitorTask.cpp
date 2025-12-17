#include "SystemMonitorTask.h"
#include "DisplayTask.h"
#include "PlatformConfig.h"
#include "hal/PlatformFactory.h"
#include "hal/SDCardAPI.h"
#include "utility/TimeUtils.hpp"
#include "hal/display/DisplayQueue.h"
#include "freertos/LogManager.h"
#include "PatternManager.h"

// Static render function for display ownership
void SystemMonitorTask::renderSystemStats(SSD1306_Display& display) {
    // Safety check: Don't render if system isn't ready
    if (millis() < SecondsToMs(1)) {
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
    
    // Title - moved up
    display.printCentered(5, "System Status", 1);
    
    // Uptime - split across two lines (formatted as days/hours/minutes/seconds)
    // Calculate days, hours, minutes, seconds using simple integer arithmetic
    // This avoids floating point and potential overflow issues
    uint32_t days = uptime / 86400;        // 86400 seconds per day
    uint32_t hours = (uptime % 86400) / 3600;  // 3600 seconds per hour
    uint32_t minutes = (uptime % 3600) / 60;   // 60 seconds per minute
    uint32_t seconds = uptime % 60;            // remaining seconds
    
    // First line: "Uptime: Xd Xh"
    char uptimeLine1[32];
    snprintf(uptimeLine1, sizeof(uptimeLine1), "Uptime: %u d %u h", 
             (unsigned int)days, (unsigned int)hours);
    display.printAt(2, 15, uptimeLine1, 1);
    
    // Second line: "        Xm Xs" (indented to align with numbers on first line)
    char uptimeLine2[32];
    snprintf(uptimeLine2, sizeof(uptimeLine2), "        %u m %u s", 
             (unsigned int)minutes, (unsigned int)seconds);
    display.printAt(2, 25, uptimeLine2, 1);
    
    // Heap usage - moved up
    char heapText[32];
    snprintf(heapText, sizeof(heapText), "Heap: %d%% (%dKB)", heapUsagePercent, freeHeap/1024);
    display.printAt(2, 35, heapText, 1);
    
    // Power monitoring display with rolling average
#if SUPPORTS_POWER_SENSORS
    static float currentAverage = 0.0f;
    static float powerAverage = 0.0f;
    static uint32_t lastUpdate = 0;
    const float alpha = 0.1f; // Smoothing factor (0.1 = 10% new, 90% old)
    
    if (g_currentSensor && g_voltageSensor) {
        float current_mA = g_currentSensor->readCurrentDCFiltered_mA();
        float voltage_V = g_voltageSensor->readVoltageDCFiltered_V();
        float power_W = (current_mA / 1000.0f) * voltage_V;  // Instant power calculation
        
        // Update rolling averages
        if (millis() - lastUpdate > 100) { // Update every 100ms
            if (currentAverage == 0.0f) {
                currentAverage = current_mA; // Initialize
                powerAverage = power_W; // Initialize
            } else {
                currentAverage = (alpha * current_mA) + ((1.0f - alpha) * currentAverage);
                powerAverage = (alpha * power_W) + ((1.0f - alpha) * powerAverage);
            }
            lastUpdate = millis();
        }
        
        // Only show power data if sensors are initialized
        if (current_mA != -999.0f && voltage_V > 0.0f) {
            // Current average and voltage on one line
            char currentText[32];
            snprintf(currentText, sizeof(currentText), "I:%.0fmA V:%.1fV", currentAverage, voltage_V);
            display.printAt(2, 35, currentText, 1);
            
            // Power consumption (rolling average)
            char powerText[32];
            snprintf(powerText, sizeof(powerText), "P:%.1fW", powerAverage);
            display.printAt(2, 45, powerText, 1);
        } else {
            // Fallback if power sensors not available
            char powerText[32];
            snprintf(powerText, sizeof(powerText), "Power: N/A");
            display.printAt(2, 35, powerText, 1);
        }
    } else {
        // Fallback if global sensors not available
        char powerText[32];
        snprintf(powerText, sizeof(powerText), "Power: N/A");
        display.printAt(2, 35, powerText, 1);
    }
#else
    // Power sensors not supported on this platform
    // char powerText[32];
    // snprintf(powerText, sizeof(powerText), "Power: --");
    // display.printAt(2, 35, powerText, 1);
#endif
    
    // System status (tasks, CPU, temperature) - moved up to fit on screen
    char statusText[32];
    float temperature = g_temperatureSensor->getTemperatureF();
    snprintf(statusText, sizeof(statusText), "Ts:%d %dMH %.0fF", taskCount, cpuFreq, temperature);
    display.printAt(2, 55, statusText, 1);
}

void SystemMonitorTask::updateDisplay() {
    static uint32_t ownershipStartTime = 0;
    static bool ownershipActive = false;
    
    // Safety check: Don't try to access display too early in boot
    static uint32_t bootTime = millis();
    if (bootTime < SecondsToMs(1)) {  // Wait 1 second after boot before trying display
        return;
    }

    const auto now = millis();
    
    // Try to request display ownership
    if (!ownershipActive && now - _lastDisplayRelease >= _minDisplayReleaseTime && DisplayTask::requestOwnership("SystemMonitor", renderSystemStats)) {
        ownershipActive = true;
        ownershipStartTime = now;
        LOG_DEBUGF("SystemMonitorTask started display ownership");
    } else if (ownershipActive) {
        // Check if it's time to release ownership
        if (now - ownershipStartTime >= _displayOwnershipTime ) {
            // Release ownership after 500ms
            if (DisplayTask::releaseOwnership("SystemMonitor")) {
                LOG_DEBUGF("SystemMonitorTask released display ownership");
            }
            ownershipActive = false;
            _lastDisplayRelease = now;
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
    logPowerConsumption();
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

void SystemMonitorTask::initializePowerSensors() {
#if SUPPORTS_POWER_SENSORS
    LOG_INFO("Power sensors are now initialized globally in main.cpp");
    LOG_INFO("Using global g_currentSensor and g_voltageSensor instances");
    
    // Check if global sensors are available
    if (g_currentSensor && g_voltageSensor) {
        LOG_INFO("Global power sensors are available and ready");
        _powerSensorsInitialized = true;
    } else {
        LOG_WARN("Global power sensors not available - will retry on next cycle");
        _powerSensorsInitialized = false;
    }
#else
    LOG_INFO("Power sensors not supported on this platform");
    _powerSensorsInitialized = false;
#endif
}

float SystemMonitorTask::getCurrentDraw_mA() {
#if SUPPORTS_POWER_SENSORS
    if (!_powerSensorsInitialized || !g_currentSensor) {
        return -1.0f;  // Error value
    }
    
    // Use global current sensor's filtered reading method
    return g_currentSensor->readCurrentDCFiltered_mA();
#else
    return -1.0f;  // Not supported
#endif
}

float SystemMonitorTask::getSupplyVoltage_V() {
#if SUPPORTS_POWER_SENSORS
    if (!_powerSensorsInitialized || !g_voltageSensor) {
        return -1.0f;  // Error value
    }
    
    // Use global voltage sensor's filtered reading
    return g_voltageSensor->readVoltageDCFiltered_V();
#else
    return -1.0f;  // Not supported
#endif
}

float SystemMonitorTask::getPowerConsumption_W() {
#if SUPPORTS_POWER_SENSORS
    if (!_powerSensorsInitialized) {
        return -1.0f;  // Error value
    }
    float current_A = getCurrentDraw_mA() / 1000.0f;
    float voltage_V = getSupplyVoltage_V();
    return current_A * voltage_V;
#else
    return -1.0f;  // Not supported
#endif
}

void SystemMonitorTask::logPowerConsumption() {
#if SUPPORTS_POWER_SENSORS
    if (!_powerSensorsInitialized) {
        LOG_WARN("Power sensors not initialized, skipping power monitoring");
        return;
    }
    
    // Get real power measurements
    float current_mA = getCurrentDraw_mA();
    float voltage_V = getSupplyVoltage_V();
    float power_W = getPowerConsumption_W();
    
    // Get system state for context
    uint32_t cpuFreq = PlatformFactory::getCpuFreqMHz();
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    
    // Get radio states for correlation
    bool wifiEnabled = false;
    #if SUPPORTS_WIFI
    wifiEnabled = WiFi.status() != WL_DISCONNECTED;
    #endif
    
    bool bleEnabled = false;
    #if SUPPORTS_BLE
    bleEnabled = BLE.connected() || BLE.advertise();
    #endif
    
    // Log comprehensive power information
    LOG_PRINTF("Power Monitoring - Current: %.1fmA, Voltage: %.2fV, Power: %.2fW", 
               current_mA, voltage_V, power_W);
    LOG_PRINTF("System State - CPU: %dMHz, Tasks: %d, WiFi: %s, BLE: %s", 
               cpuFreq, taskCount, wifiEnabled ? "ON" : "OFF", bleEnabled ? "ON" : "OFF");
    
    // SMART POWER REACTIONS
    static float lastCurrent = 0.0f;
    static float lastVoltage = 0.0f;
    static float lastPower = 0.0f;
    static uint32_t lastAlertTime = 0;
    const uint32_t ALERT_COOLDOWN = 10000; // 10 seconds between alerts
    
    // Calculate deltas
    float currentDelta = current_mA - lastCurrent;
    float voltageDelta = voltage_V - lastVoltage;
    float powerDelta = power_W - lastPower;
    
    // Store current values for next comparison
    lastCurrent = current_mA;
    lastVoltage = voltage_V;
    lastPower = power_W;
    
    // Only react if enough time has passed since last alert
    if (millis() - lastAlertTime < ALERT_COOLDOWN) {
        return;
    }
    
    // REACTION 1: High Current Alert (>3A for LED strips)
    if (current_mA > 3000.0f) {
        LOG_WARNF("🚨 HIGH CURRENT ALERT: %.1fmA (>3A)", current_mA);
        LOG_WARN("Possible causes: LED strip short, too many LEDs, power supply issue");
        DisplayQueue::getInstance().safeRequestBannerMessage("POWER", "HIGH CURRENT!");
        lastAlertTime = millis();
    }
    
    // REACTION 2: Low Voltage Alert (<4.5V)
    if (voltage_V < 4.5f) {
        LOG_WARNF("⚠️ LOW VOLTAGE ALERT: %.2fV (<4.5V)", voltage_V);
        LOG_WARN("Possible causes: Power supply overload, wiring resistance, brown-out risk");
        DisplayQueue::getInstance().safeRequestBannerMessage("POWER", "LOW VOLTAGE!");
        lastAlertTime = millis();
    }
    
    // REACTION 3: Power Spike Detection (>50% increase in 1 second)
    if (abs(powerDelta) > (lastPower * 0.5f) && lastPower > 1.0f) {
        LOG_WARNF("⚡ POWER SPIKE DETECTED: %.2fW → %.2fW (%.1f%% change)", 
                  lastPower, power_W, (powerDelta/lastPower)*100.0f);
        LOG_WARN("Possible causes: LED pattern change, system load spike, sensor issue");
        DisplayQueue::getInstance().safeRequestBannerMessage("POWER", "SPIKE!");
        lastAlertTime = millis();
    }
    
    // REACTION 4: Current Drop Detection (LEDs disconnected?)
    if (currentDelta < -1000.0f && current_mA < 500.0f) {
        LOG_INFOF("🔌 LED DISCONNECTION DETECTED: Current dropped %.1fmA", abs(currentDelta));
        LOG_INFO("LEDs may have been disconnected or power interrupted");
        DisplayQueue::getInstance().safeRequestBannerMessage("POWER", "LEDS OFF");
        lastAlertTime = millis();
    }
    
    // REACTION 5: Current Surge Detection (LEDs connected?)
    if (currentDelta > 1000.0f && current_mA > 500.0f) {
        LOG_INFOF("🔌 LED CONNECTION DETECTED: Current increased %.1fmA", currentDelta);
        LOG_INFO("LEDs may have been connected or pattern changed");
        DisplayQueue::getInstance().safeRequestBannerMessage("POWER", "LEDS ON");
        lastAlertTime = millis();
    }
    
    // REACTION 6: Voltage Instability (>0.5V fluctuation)
    if (abs(voltageDelta) > 0.5f) {
        LOG_WARNF("📊 VOLTAGE INSTABILITY: %.2fV → %.2fV (%.2fV change)", 
                  lastVoltage, voltage_V, voltageDelta);
        LOG_WARN("Possible causes: Power supply regulation, load changes, wiring issues");
        lastAlertTime = millis();
    }
    
    // REACTION 7: Power Efficiency Monitoring
    static float totalEnergy = 0.0f;
    static uint32_t lastEnergyTime = millis();
    uint32_t now = millis();
    float timeDelta = (now - lastEnergyTime) / 1000.0f; // seconds
    totalEnergy += power_W * timeDelta; // watt-seconds
    lastEnergyTime = now;
    
    // Log energy consumption every 5 minutes
    static uint32_t lastEnergyLog = 0;
    if (now - lastEnergyLog > 300000) { // 5 minutes
        LOG_PRINTF("⚡ ENERGY CONSUMPTION: %.2f Wh (%.2f kWh) over %.1f hours", 
                   totalEnergy / 3600.0f, totalEnergy / 3600000.0f, 
                   (now / 1000.0f) / 3600.0f);
        lastEnergyLog = now;
    }
    
    // Print raw sensor diagnostics occasionally for debugging
    static uint32_t lastDiagnostic = 0;
    if (millis() - lastDiagnostic > 60000) {  // Every minute
        LOG_DEBUG("=== Power Sensor Diagnostics (Simplified Wrapper Classes) ===");
        
        // Current sensor diagnostics
        if (g_currentSensor) {
            g_currentSensor->printDiagnostics();
        }
        
        // Voltage sensor diagnostics
        if (g_voltageSensor) {
            g_voltageSensor->printDiagnostics();
        }
        
        lastDiagnostic = millis();
    }
#else
    // Power sensors not supported on this platform
    LOG_DEBUG("Power monitoring not supported on this platform");
#endif
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

SystemMonitorTask::SystemMonitorTask(uint32_t intervalMs)
    : SRTask("SysMonitor", 4096, tskIDLE_PRIORITY + 1, 0),  // Core 0
      _intervalMs(intervalMs),
      _displayUpdateInterval(1000),  // Display updates every 1 second
      _lastDisplayUpdate(0),
      _lastDisplayRelease(0),
      _powerSensorsInitialized(false) {
}
