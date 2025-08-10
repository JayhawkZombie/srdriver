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
    
    // Power monitoring (if available) and temperature
    char statusText[32];
    float temperature = g_temperatureSensor->getTemperatureF();
    
    // Try to get power data (this may not work if called from static context)
    // For now, show tasks, CPU, and temperature
    snprintf(statusText, sizeof(statusText), "Ts:%d %dMH %.1fF", taskCount, cpuFreq, temperature);
    display.printAt(2, 55, statusText, 1);  // Moved up from 60
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
    LOG_INFO("Initializing power sensors using simplified ACS712 wrapper classes...");
    
    // Initialize current sensor with library backend (30A sensor, pin A2)
    // BACK TO 30A: 5A config just amplified offset, not real current
    _currentSensor = new ACS712CurrentSensor(A2, ACS712_30A, 5.0f, 3.3f);
    _currentSensor->begin();
    
    // Disable polarity correction since raw readings are already correct for LED current draw
    _currentSensor->setPolarityCorrection(false);
    LOG_INFO("Disabled current sensor polarity correction - raw readings are correct");
    
    // Let the library handle calibration automatically - don't override with manual midpoint
    LOG_INFO("Using library auto-calibration - midpoint should be set by autoMidPoint()");
    
    // Print initial diagnostics
    _currentSensor->printDiagnostics();
    
    // CRITICAL DEBUGGING: Check if current is actually flowing through sensor
    LOG_INFO("=== CRITICAL DEBUGGING: Current Path Analysis ===");
    LOG_INFO("BACK TO ACS712_30A (66mV/A sensitivity)");
    LOG_INFO("5A config showed readings don't change when LEDs removed = WIRING ISSUE");
    LOG_INFO("LEDs must be powered through path that BYPASSES the ACS712 sensor");
    LOG_INFO("Check: Are ALL LED power connections going THROUGH the sensor?");
    
    // Simple connectivity test: Take multiple readings to see if sensor responds to ANYTHING
    LOG_INFO("=== ACS712 CONNECTIVITY TEST ===");
    for (int i = 0; i < 5; i++) {
        delay(100);
        float reading = _currentSensor->readCurrentDC_mA();
        LOG_PRINTF("Test reading %d: %.1f mA", i+1, reading);
    }
    LOG_INFO("If all readings are identical, sensor may not be measuring real current");
    LOG_INFO("Try physically disconnecting sensor power to see if readings change to zero");
    
    // Initialize voltage sensor with simplified interface (pin A3, proven 5.27:1 ratio)
    _voltageSensor = new ACS712VoltageSensor(A3, 3.3f, 5.27f);
    _voltageSensor->begin();
    
    _powerSensorsInitialized = true;
    
    LOG_INFO("Power sensors initialized successfully with simplified classes");
}

float SystemMonitorTask::getCurrentDraw_mA() {
    if (!_powerSensorsInitialized || !_currentSensor) {
        return -1.0f;  // Error value
    }
    
    // Use our wrapper's filtered reading method
    return _currentSensor->readCurrentDCFiltered_mA();
}

float SystemMonitorTask::getSupplyVoltage_V() {
    if (!_powerSensorsInitialized || !_voltageSensor) {
        return -1.0f;  // Error value
    }
    
    // Use our simplified voltage sensor's filtered reading
    return _voltageSensor->readVoltageDCFiltered_V();
}

float SystemMonitorTask::getPowerConsumption_W() {
    if (!_powerSensorsInitialized) {
        return -1.0f;  // Error value
    }
    float current_A = getCurrentDraw_mA() / 1000.0f;
    float voltage_V = getSupplyVoltage_V();
    return current_A * voltage_V;
}

void SystemMonitorTask::logPowerConsumption() {
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
    
    // Power analysis and warnings
    if (current_mA > 800.0f) {
        LOG_WARNF("High current draw detected: %.1fmA", current_mA);
        LOG_INFO("High current may indicate:");
        LOG_INFO("- High CPU activity or intensive processing");
        LOG_INFO("- Active radio connections (normal operation)");
        LOG_INFO("- Memory operations or LED activity");
    }
    
    if (voltage_V < 4.7f) {
        LOG_WARNF("Low supply voltage detected: %.2fV", voltage_V);
        LOG_WARN("Low voltage may cause:");
        LOG_WARN("- System instability");
        LOG_WARN("- Brown-out resets");
        LOG_WARN("- Check power supply capacity");
    }
    
    if (voltage_V > 5.3f) {
        LOG_WARNF("High supply voltage detected: %.2fV", voltage_V);
        LOG_WARN("High voltage may indicate:");
        LOG_WARN("- Power supply regulation issues");
        LOG_WARN("- Potential component damage risk");
    }
    
    if (power_W > 3.0f) {  // Raised threshold since 2.5W might be normal
        LOG_WARNF("High power consumption: %.2fW", power_W);
        LOG_INFO("This may be normal for a full-featured system with:");
        LOG_INFO("- Active WiFi and BLE");
        LOG_INFO("- LED patterns running");
        LOG_INFO("- Multiple active tasks");
        LOG_INFO("Consider monitoring trends rather than absolute values");
    }
    
    // Store historical power data for trend analysis
    static float lastCurrent = 0.0f;
    static float lastVoltage = 0.0f;
    
    if (lastCurrent != 0.0f) {
        float currentDelta = current_mA - lastCurrent;
        float voltageDelta = voltage_V - lastVoltage;
        
        if (abs(currentDelta) > 100.0f) {
            LOG_PRINTF("Power change detected - Current delta: %.1fmA, Voltage delta: %.2fV", 
                      currentDelta, voltageDelta);
        }
    }
    
    lastCurrent = current_mA;
    lastVoltage = voltage_V;
    
    // Print raw sensor diagnostics occasionally for debugging
    static uint32_t lastDiagnostic = 0;
    if (millis() - lastDiagnostic > 60000) {  // Every minute
        LOG_DEBUG("=== Power Sensor Diagnostics (Simplified Wrapper Classes) ===");
        
        // Current sensor diagnostics
        if (_currentSensor) {
            _currentSensor->printDiagnostics();
        }
        
        // Voltage sensor diagnostics
        if (_voltageSensor) {
            _voltageSensor->printDiagnostics();
        }
        
        lastDiagnostic = millis();
    }
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
      _currentSensor(nullptr),
      _voltageSensor(nullptr),
      _powerSensorsInitialized(false) {
}
