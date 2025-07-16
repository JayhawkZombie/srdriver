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
    
    _display.clear();
    uint32_t clearTime = micros();
    
    // Render banner in yellow region (top ~12 pixels)
    renderBanner();
    uint32_t bannerTime = micros();
    
    // Draw separator line
    _display.drawLine(0, 12, 128, 12, COLOR_WHITE);
    uint32_t lineTime = micros();
    
    // Status info in main display area
    _display.setTextColor(COLOR_WHITE);
    _display.setTextSize(1);
    
    // Show system status
    _display.printAt(2, 20, "System: Running", 1);
    
    // Show memory info
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    char memText[32];
    snprintf(memText, sizeof(memText), "Memory: %d/%d KB", freeHeap/1024, totalHeap/1024);
    _display.printAt(2, 30, memText, 1);
    
    // Show task count
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    char taskText[32];
    snprintf(taskText, sizeof(taskText), "Tasks: %d", taskCount);
    _display.printAt(2, 40, taskText, 1);
    
    // Show queue status
    char queueText[32];
    if (_displayQueue.hasActiveMessage()) {
        snprintf(queueText, sizeof(queueText), "Queue: Active");
    } else {
        snprintf(queueText, sizeof(queueText), "Queue: Idle");
    }
    _display.printAt(2, 50, queueText, 1);
    uint32_t textTime = micros();
    
    // Draw a simple animation - bouncing dot
    uint8_t dotX = 64 + 30 * sin(_frameCount * 0.1);
    uint8_t dotY = 55 + 10 * cos(_frameCount * 0.15);
    _display.fillCircle(dotX, dotY, 2, COLOR_WHITE);
    uint32_t animationTime = micros();
    
    // Show the display
    _display.show();
    uint32_t showTime = micros();
    
    // Log detailed timing every 100 frames
    static uint32_t frameCounter = 0;
    frameCounter++;
    if (frameCounter % 100 == 0) {
        LOG_DEBUGF("Display timing breakdown (μs): Timeout=%d, Clear=%d, Banner=%d, Line=%d, Text=%d, Anim=%d, Show=%d, Total=%d",
                   timeoutTime - startTime,
                   clearTime - timeoutTime,
                   bannerTime - clearTime,
                   lineTime - bannerTime,
                   textTime - lineTime,
                   animationTime - textTime,
                   showTime - animationTime,
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
        // Show compact system stats in the banner
        renderCompactStats();
    }
    uint32_t renderTime = micros();
    
    // Log banner timing every 200 frames (less frequent to avoid spam)
    static uint32_t bannerCounter = 0;
    bannerCounter++;
    if (bannerCounter % 200 == 0) {
        LOG_DEBUGF("Banner timing (μs): Queue=%d, Render=%d, Total=%d",
                   queueTime - startTime, renderTime - queueTime, renderTime - startTime);
    }
}

void DisplayTask::renderCompactStats() {
    // Get system stats
    uint32_t uptime = millis() / 1000;  // Convert to seconds
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint8_t heapPercent = (freeHeap * 100) / totalHeap;
    
    // Format time more compactly
    char timeStr[8];
    if (uptime < 60) {
        snprintf(timeStr, sizeof(timeStr), "%ds", uptime);
    } else if (uptime < 3600) {
        snprintf(timeStr, sizeof(timeStr), "%dm", uptime / 60);
    } else {
        snprintf(timeStr, sizeof(timeStr), "%dh", uptime / 3600);
    }
    
    // Choose different stat combinations based on what's most useful
    static uint32_t lastStatRotation = 0;
    uint32_t now = millis();
    
    // Rotate through different stat combinations every 5 seconds
    uint8_t statMode = (now / 5000) % 3;
    
    // Pre-calculate values used in multiple cases
    uint32_t freeHeapKB = freeHeap / 1024;
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    uint32_t cpuFreq = ESP.getCpuFreqMHz();
    
    char statsText[32];
    if (statMode == 0) {
        // Mode 1: Time | Heap% | Frame ms
        snprintf(statsText, sizeof(statsText), "%s | %d%% | %dms", 
                 timeStr, heapPercent, (int)(_averageFrameTime / 1000));
    } else if (statMode == 1) {
        // Mode 2: Heap KB | Tasks | Frame ms
        snprintf(statsText, sizeof(statsText), "%dKB | %dT | %dms", 
                 freeHeapKB, taskCount, (int)(_averageFrameTime / 1000));
    } else {
        // Mode 3: CPU MHz | Heap% | Frame ms
        snprintf(statsText, sizeof(statsText), "%dMHz | %d%% | %dms", 
                 cpuFreq, heapPercent, (int)(_averageFrameTime / 1000));
    }
    
    // Render in banner
    _display.printCentered(2, statsText, 1);
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