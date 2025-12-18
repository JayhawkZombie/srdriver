#include "OLEDDisplayTask.h"
#include "DeviceInfo.h"

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
    
    // Clear the display buffer
    _display.clear();
    
    // Render banner in yellow region (top ~12 pixels)
    renderBanner();
    
    // Draw separator line
    _display.drawLine(0, 12, 128, 12, COLOR_WHITE);
    
    // Render default content
    renderDefaultContent();
    
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

