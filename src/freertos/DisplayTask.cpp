#include "DisplayTask.h"

void DisplayTask::run() {
    LOG_INFO("Display task started");
    LOG_PRINTF("Update interval: %d ms (~%d FPS)", 
               _updateInterval, 1000 / _updateInterval);
    
    // Signal that DisplayTask is now ready to handle queue requests
    _displayQueue.setDisplayState(DisplayQueue::DisplayState::READY);
    LOG_INFO("Display system ready - queue requests now accepted");
    
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
            LOG_DEBUGF("Display Update - Frames: %d, Interval: %d ms", 
                      _frameCount, _updateInterval);
            _frameCount = 0;
            lastLogTime = now;
        }
        
        // Sleep until next update
        SRTask::sleepUntil(&lastWakeTime, _updateInterval);
    }
}

void DisplayTask::updateDisplay() {
    // Check for message timeouts first
    _displayQueue.checkMessageTimeout();
    
    // Clear the display buffer
    _display.clear();
    
    // 1. Render main display area (blue region)
    renderMainDisplay();
    
    // 2. Render banner (yellow region) - always on top
    renderBanner();
    
    // 3. Upload to hardware
    _display.show();
}

void DisplayTask::renderMainDisplay() {
    // Check if there's an active main display request
    if (_displayQueue.hasActiveMainDisplay()) {
        const DisplayBuffer* buffer = _displayQueue.getCurrentMainDisplay();
        if (buffer && buffer->isReady()) {
            // Upload the buffer to the display
            uploadBufferToDisplay(*buffer);
        }
    } else {
        // No main display request - show default content
        // This could be a simple "No Content" message or blank
        _display.printAt(2, 20, "No Display Content", 1);
    }
}

void DisplayTask::renderBanner() {
    _display.setTextColor(COLOR_WHITE);
    _display.setTextSize(1);
    
    // Get banner text from queue
    String bannerText = _displayQueue.getFullBannerText();
    
    // Always render banner in yellow region (top ~12 pixels)
    _display.printCentered(2, bannerText.c_str(), 1);
}

void DisplayTask::uploadBufferToDisplay(const DisplayBuffer& buffer) {
    // Get the buffer data
    const uint8_t* bufferData = buffer.getBuffer();
    
    // For now, we'll use the existing display API
    // In the future, we could add a direct buffer upload method to SSD1306_Display
    
    // Clear the display first
    _display.clear();
    
    // Upload the buffer data to the display
    // This is a simplified approach - in a full implementation,
    // we'd want to add a direct buffer upload method to the display driver
    
    // For now, we'll just mark that we've processed the buffer
    // The actual buffer upload will be handled by the display driver
    // when we call _display.show()
    
    // TODO: Add direct buffer upload method to SSD1306_Display
    // _display.uploadBuffer(bufferData);
} 