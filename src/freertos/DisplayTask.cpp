#include "DisplayTask.h"

// Static member definitions
String DisplayTask::_currentOwner = "";
DisplayRenderCallback DisplayTask::_currentRenderCallback = nullptr;

// Ownership management methods
bool DisplayTask::requestOwnership(const String& taskName, DisplayRenderCallback renderCallback) {
    // Safety check: Don't accept requests until DisplayTask is fully initialized
    static bool displayTaskReady = false;
    static uint32_t initTime = millis();
    
    // Mark as ready after 3 seconds of operation
    if (!displayTaskReady && (millis() - initTime) > 3000) {
        displayTaskReady = true;
        LOG_DEBUGF("DisplayTask ready to accept ownership requests");
    }
    
    if (!displayTaskReady) {
        return false;  // Not ready yet
    }
    
    if (_currentOwner.length() > 0) {
        // Only log denied requests occasionally to avoid spam
        static uint32_t lastDeniedLog = 0;
        uint32_t now = millis();
        if (now - lastDeniedLog > 5000) {  // Log every 5 seconds max
            LOG_DEBUGF("Display ownership request denied: %s (currently owned by: %s)", 
                      taskName.c_str(), _currentOwner.c_str());
            lastDeniedLog = now;
        }
        return false;
    }
    
    _currentOwner = taskName;
    _currentRenderCallback = renderCallback;
    LOG_DEBUGF("Display ownership granted to: %s", taskName.c_str());
    return true;
}

bool DisplayTask::releaseOwnership(const String& taskName) {
    if (_currentOwner != taskName) {
        LOG_DEBUGF("Display ownership release denied: %s (currently owned by: %s)", 
                  taskName.c_str(), _currentOwner.c_str());
        return false;
    }
    
    LOG_DEBUGF("Display ownership released by: %s", taskName.c_str());
    _currentOwner = "";
    _currentRenderCallback = nullptr;
    return true;
}

void DisplayTask::run() {
    LOG_INFO("Display task started");
    LOG_PRINTF("Update interval: %d ms (~%d FPS)", 
               _updateInterval, 1000 / _updateInterval);
    
    // Signal that DisplayTask is now ready to handle queue requests
    _displayQueue.setDisplayState(DisplayQueue::DisplayState::READY);
    LOG_INFO("Display system ready - queue requests now accepted");
    
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (true) {
        uint32_t frameStartTime = micros();
        
        // Update display
        updateDisplay();
        
        // Calculate frame time
        uint32_t frameTime = micros() - frameStartTime;
        updatePerformanceMetrics(frameTime);
        
        // Increment frame counter
        _frameCount++;
        
        // Log status every 10 seconds
        static uint32_t lastLogTime = 0;
        uint32_t now = millis();
        if (now - lastLogTime > 10000) {
            LOG_DEBUGF("Display Update - Frames: %d, Interval: %d ms", 
                      _frameCount, _updateInterval);
            
            // Log performance metrics
            if (!isPerformanceAcceptable()) {
                LOG_WARNF("Display performance warning: %s", getPerformanceReport().c_str());
            } else {
                LOG_DEBUGF("Display performance: %s", getPerformanceReport().c_str());
            }
            
            _frameCount = 0;
            lastLogTime = now;
        }
        
        // Sleep until next update
        SRTask::sleepUntil(&lastWakeTime, _updateInterval);
    }
}

void DisplayTask::updateDisplay() {
    uint32_t startTime = micros();
    
    // Check for message timeouts first
    _displayQueue.checkMessageTimeout();
    uint32_t timeoutTime = micros();
    
    // Check if a task owns the display BEFORE any rendering
    bool hasOwner = (_currentOwner.length() > 0 && _currentRenderCallback != nullptr);
    
    // Clear the display buffer completely - try multiple times to ensure it's cleared
    _display.clear();
    _display.clear();  // Double clear to ensure buffer is empty
    uint32_t clearTime = micros();
    
    // Render banner in yellow region (top ~12 pixels)
    renderBanner();
    uint32_t bannerTime = micros();
    
    // Draw separator line
    _display.drawLine(0, 12, 128, 12, COLOR_WHITE);
    uint32_t lineTime = micros();
    
    // Render main content based on ownership
    if (hasOwner) {
        // Call the owner's render function
        _currentRenderCallback(_display);
    } else {
        // Render default content (animating ball and system info)
        renderDefaultContent();
    }
    uint32_t contentTime = micros();
    
    // Show the display
    _display.show();
    uint32_t showTime = micros();
    
    // Log detailed timing every 1000 frames (much less frequent)
    static uint32_t frameCounter = 0;
    frameCounter++;
    if (frameCounter % 1000 == 0) {
        LOG_DEBUGF("Display timing breakdown (μs): Timeout=%d, Clear=%d, Banner=%d, Line=%d, Content=%d, Show=%d, Total=%d",
                   timeoutTime - startTime,
                   clearTime - timeoutTime,
                   bannerTime - clearTime,
                   lineTime - bannerTime,
                   contentTime - lineTime,
                   showTime - contentTime,
                   showTime - startTime);
    }
}

void DisplayTask::renderBanner() {
    uint32_t startTime = micros();
    
    _display.setTextColor(COLOR_WHITE);
    _display.setTextSize(1);
    
    // Get banner text from queue
    String bannerText = _displayQueue.getFullBannerText();
    uint32_t queueTime = micros();
    
    // If there's an active message, show it
    if (_displayQueue.hasActiveMessage()) {
        _display.printCentered(2, bannerText.c_str(), 1);
    } else {
        // Show simple "SRDriver" text when no banner message
        _display.printCentered(2, "SRDriver", 1);
    }
    uint32_t renderTime = micros();
    
    // Log banner timing every 1000 frames (much less frequent to avoid spam)
    static uint32_t bannerCounter = 0;
    bannerCounter++;
    if (bannerCounter % 1000 == 0) {
        LOG_DEBUGF("Banner timing (μs): Queue=%d, Render=%d, Total=%d",
                   queueTime - startTime, renderTime - queueTime, renderTime - startTime);
    }
}

void DisplayTask::renderDefaultContent() {
    // Only render a simple animating ball - no system stats
    uint8_t dotX = 64 + 30 * sin(_frameCount * 0.1);
    uint8_t dotY = 55 + 10 * cos(_frameCount * 0.15);
    _display.fillCircle(dotX, dotY, 2, COLOR_WHITE);
}

void DisplayTask::updatePerformanceMetrics(uint32_t frameTime) {
    // Update max frame time
    if (frameTime > _maxFrameTime) {
        _maxFrameTime = frameTime;
    }
    
    // Update average frame time (rolling average with limited samples)
    const uint32_t maxSamples = 100;  // Limit samples to prevent stale data
    if (_performanceSampleCount < maxSamples) {
        _performanceSampleCount++;
    }
    
    // Calculate rolling average
    _averageFrameTime = (_averageFrameTime * (_performanceSampleCount - 1) + frameTime) / _performanceSampleCount;
    
    // Calculate frame rate from current average frame time
    if (_averageFrameTime > 0) {
        _frameRate = 1000000.0f / _averageFrameTime;  // FPS = 1,000,000 μs / average frame time
    }
    
    // Check for missed frames (frame time > target interval)
    if (frameTime > (_updateInterval * 1000)) {  // Convert ms to μs
        _missedFrames++;
    }
    
    // Reset performance metrics every 10 seconds to prevent stale data
    static uint32_t lastResetTime = 0;
    uint32_t now = millis();
    if (now - lastResetTime > 10000) {  // Every 10 seconds
        _performanceSampleCount = 0;
        _averageFrameTime = 0;
        _maxFrameTime = 0;
        _missedFrames = 0;
        lastResetTime = now;
        LOG_DEBUG("Performance metrics reset");
    }
}

bool DisplayTask::isPerformanceAcceptable() const {
    // Performance is acceptable if:
    // 1. Average frame time is less than 95% of target interval (was 80%)
    // 2. Max frame time is less than 200% of target interval (was 150%)
    // 3. Missed frames are less than 10% of total frames (was 5%)
    // 4. Frame rate is at least 70% of target (was 80%)
    
    uint32_t targetFrameTime = _updateInterval * 1000;  // Convert to μs
    float targetFrameRate = 1000000.0f / targetFrameTime;
    
    bool avgTimeOK = _averageFrameTime < (targetFrameTime * 0.95f);
    bool maxTimeOK = _maxFrameTime < (targetFrameTime * 2.0f);
    bool missedFramesOK = (_frameCount == 0) || (_missedFrames < (_frameCount * 0.10f));
    bool frameRateOK = _frameRate > (targetFrameRate * 0.7f);
    
    return avgTimeOK && maxTimeOK && missedFramesOK && frameRateOK;
}

String DisplayTask::getPerformanceReport() const {
    char buffer[256];
    uint32_t targetFrameTime = _updateInterval * 1000;  // Convert to μs
    float targetFrameRate = 1000000.0f / targetFrameTime;
    
    snprintf(buffer, sizeof(buffer), 
             "Avg: %dms, Max: %dms, Target: %dms, FPS: %.1f, Missed: %d/%d", 
             (int)(_averageFrameTime / 1000), (int)(_maxFrameTime / 1000), (int)(targetFrameTime / 1000),
             _frameRate, _missedFrames, _frameCount);
    
    return String(buffer);
} 