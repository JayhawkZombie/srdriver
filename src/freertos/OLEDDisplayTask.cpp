#include "OLEDDisplayTask.h"
#include "DeviceInfo.h"
#include "../config/JsonSettings.h"
#include "../utility/StringUtils.h"

// Forward declaration
extern SystemMonitorTask* g_systemMonitorTask;

OLEDDisplayTask::OLEDDisplayTask(const JsonSettings* settings,
                                 uint32_t updateIntervalMs,
                                 uint32_t stackSize,
                                 UBaseType_t priority,
                                 BaseType_t core)
    : SRTask("OLEDDisplay", stackSize, priority, core),
      _display(),
      _displayQueue(DisplayQueue::getInstance()),
      _updateInterval(updateIntervalMs),
      _frameCount(0),
      _viewSwitchInterval(5000),  // Switch views every 5 seconds
      _lastViewSwitch(0),
      _showStats(false)
{
    // Initialize display address from settings if provided
    if (settings) {
        const auto displaySettings = settings->_doc["display"];
        if (!displaySettings.isNull()) {
            const auto addressSetting = displaySettings["address"];
            if (!addressSetting.isNull()) {
                uint8_t address = hexToUint8(addressSetting.as<String>());
                _display.setAddress(address);
            }
        }
    }
    
    // Setup the display hardware
    _display.setupDisplay();
    
    // Initialize DisplayQueue in STARTUP state
    _displayQueue.setDisplayState(DisplayQueue::DisplayState::STARTUP);
}

void OLEDDisplayTask::run() {
    LOG_INFO_COMPONENT("OLEDDisplay", "OLED display task started");
    LOG_INFOF_COMPONENT("OLEDDisplay", "Update interval: %d ms (~%d FPS)", 
               _updateInterval, 1000 / _updateInterval);
    
    // Signal that display is now ready to handle queue requests
    _displayQueue.setDisplayState(DisplayQueue::DisplayState::READY);
    LOG_INFO_COMPONENT("OLEDDisplay", "Display system ready - queue requests now accepted");
    
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Update display
        updateDisplay();
        
        // Increment frame counter
        _frameCount++;
        
        // Log status every 10 seconds
        static uint32_t lastLogTime = 0;
        uint32_t now = millis();
        if (now - lastLogTime > 10000) {
            LOG_DEBUGF_COMPONENT("OLEDDisplay", "Display Update - Frames: %d, Interval: %d ms", 
                      _frameCount, _updateInterval);
            _frameCount = 0;
            lastLogTime = now;
        }
        
        // Sleep until next update
        SRTask::sleepUntil(&lastWakeTime, _updateInterval);
    }
}

void OLEDDisplayTask::updateDisplay() {
    // Check for message timeouts
    _displayQueue.checkMessageTimeout();
    
    // Switch views periodically
    uint32_t now = millis();
    if (now - _lastViewSwitch >= _viewSwitchInterval) {
        _showStats = !_showStats;
        _lastViewSwitch = now;
    }
    
    // Clear the display buffer
    _display.clear();
    
    // Render banner in yellow region (top ~12 pixels)
    renderBanner();
    
    // Draw separator line
    _display.drawLine(0, 12, 128, 12, COLOR_WHITE);
    
    // Render content based on current view
    if (_showStats && g_systemMonitorTask) {
        renderSystemStats();
    } else {
        renderDefaultContent();
    }
    
    // Show the display
    _display.show();
}

void OLEDDisplayTask::renderBanner() {
    _display.setTextColor(COLOR_WHITE);
    _display.setTextSize(1);
    
    // Get banner text from queue
    String bannerText = _displayQueue.getFullBannerText();
    
    // If there's an active message, show it
    if (_displayQueue.hasActiveMessage()) {
        _display.printCentered(2, bannerText.c_str(), 1);
    } else {
        // Show simple "SRDriver" text when no banner message
        _display.printCentered(2, "SRDriver", 1);
    }
}

void OLEDDisplayTask::renderDefaultContent() {
    // Render firmware version (first 15 chars) at the top
    _display.setTextColor(COLOR_WHITE);
    _display.setTextSize(1);
    
    // Get firmware version using static method
    String firmwareVersion = DeviceInfo::GetCompiledFirmwareVersion();
    
    // Truncate to first 15 characters
    if (firmwareVersion.length() > 15) {
        firmwareVersion = firmwareVersion.substring(0, 15);
    }
    
    // Display firmware version centered
    _display.printCentered(20, firmwareVersion.c_str(), 1);
    
    // Display build date
    String buildDate = DeviceInfo::getBuildDate();
    _display.printCentered(30, buildDate.c_str(), 1);
    
    // Display hw revision centered
    String deviceVersion = DeviceInfo::getDeviceVersion();
    _display.printCentered(40, deviceVersion.c_str(), 1);
}

void OLEDDisplayTask::renderSystemStats() {
    if (!g_systemMonitorTask) {
        return;
    }
    
    // Get system stats
    SystemStats stats = g_systemMonitorTask->getStats();
    
    _display.setTextColor(COLOR_WHITE);
    _display.setTextSize(1);
    
    // Calculate uptime components from total seconds
    uint32_t uptime = stats.uptimeSeconds;
    uint32_t days = uptime / 86400;        // 86400 seconds per day
    uint32_t hours = (uptime % 86400) / 3600;  // 3600 seconds per hour
    uint32_t minutes = (uptime % 3600) / 60;   // 60 seconds per minute
    uint32_t seconds = uptime % 60;            // remaining seconds
    
    // First line: "Uptime: Xd Xh"
    char uptimeLine1[32];
    snprintf(uptimeLine1, sizeof(uptimeLine1), "Uptime: %u d %u h", 
             (unsigned int)days, (unsigned int)hours);
    _display.printAt(2, 15, uptimeLine1, 1);
    
    // Second line: "        Xm Xs" (indented to align with numbers on first line)
    char uptimeLine2[32];
    snprintf(uptimeLine2, sizeof(uptimeLine2), "        %u m %u s", 
             (unsigned int)minutes, (unsigned int)seconds);
    _display.printAt(2, 25, uptimeLine2, 1);
    
    // Heap usage
    char heapText[32];
    snprintf(heapText, sizeof(heapText), "Heap: %d%% (%dKB)", 
             stats.heapUsagePercent, stats.freeHeap / 1024);
    _display.printAt(2, 35, heapText, 1);
    
    // System status (tasks, CPU, temperature) - on one line at bottom
    char statusText[32];
    if (stats.temperatureAvailable) {
        snprintf(statusText, sizeof(statusText), "Ts:%d %dMH %.0fF", 
                 stats.taskCount, stats.cpuFreqMHz, stats.temperatureF);
    } else if (stats.powerAvailable) {
        snprintf(statusText, sizeof(statusText), "Ts:%d %dMH %.1fW", 
                 stats.taskCount, stats.cpuFreqMHz, stats.power_W);
    } else {
        snprintf(statusText, sizeof(statusText), "Ts:%d %dMH ---", 
                 stats.taskCount, stats.cpuFreqMHz);
    }
    _display.printAt(2, 55, statusText, 1);
}

