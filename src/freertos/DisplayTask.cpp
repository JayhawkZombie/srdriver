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
    
    // Update system stats in the frame composer
    _frameComposer.updateSystemStats();
    
    // Get banner text from queue
    String bannerText = _displayQueue.getFullBannerText();
    
    // Get main display content (if any)
    const DisplayBuffer* mainContent = nullptr;
    if (_displayQueue.hasActiveMainDisplay()) {
        mainContent = _displayQueue.getCurrentMainDisplay();
    }
    
    // Compose complete frame with overlays
    DisplayBuffer* composedFrame = _frameComposer.composeFrame(mainContent, bannerText);
    
    // Upload the composed frame to the display
    uploadBufferToDisplay(*composedFrame);
}

void DisplayTask::uploadBufferToDisplay(const DisplayBuffer& buffer) {
    // Get the buffer data
    const uint8_t* bufferData = buffer.getBuffer();
    
    // Upload the buffer directly to the display
    _display.uploadBuffer(bufferData);
} 