#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "../hal/SSD_1306Component.h"
#include <math.h>

// Forward declarations
extern SSD1306_Display display;

/**
 * DisplayTask - Simple FreeRTOS task for OLED display management
 * 
 * Handles:
 * - Display updates and rendering
 * - Basic status display
 * - Simple animations
 */
class DisplayTask : public SRTask {
public:
    DisplayTask(uint32_t updateIntervalMs = 200,  // 5 FPS for display updates
                uint32_t stackSize = 4096,
                UBaseType_t priority = tskIDLE_PRIORITY + 2,  // Medium priority
                BaseType_t core = 0)  // Pin to core 0
        : SRTask("DisplayTask", stackSize, priority, core),
          _updateIntervalMs(updateIntervalMs),
          _frameCount(0),
          _lastStatusUpdate(0) {}
    
    /**
     * Get current frame count
     */
    uint32_t getFrameCount() const { return _frameCount; }
    
    /**
     * Get update interval
     */
    uint32_t getUpdateInterval() const { return _updateIntervalMs; }

protected:
    /**
     * Main task loop - handles display updates and rendering
     */
    void run() override {
        LOG_INFO("Display task started");
        LOG_PRINTF("Update interval: %d ms (~%d FPS)", 
                   _updateIntervalMs, 1000 / _updateIntervalMs);
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Update display
            updateDisplay();
            
            // Increment frame counter
            _frameCount++;
            
            // Log status every 10 seconds
            uint32_t now = millis();
            if (now - _lastStatusUpdate > 10000) {
                LOG_DEBUGF("Display Update - Frames: %d, Interval: %d ms", 
                          _frameCount, _updateIntervalMs);
                _frameCount = 0;
                _lastStatusUpdate = now;
            }
            
            // Sleep until next update
            SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
        }
    }

private:
    uint32_t _updateIntervalMs;
    uint32_t _frameCount;
    uint32_t _lastStatusUpdate;
    
    /**
     * Update display content
     */
    void updateDisplay() {
        display.clear();
        
        // Draw a simple status screen
        display.setTextColor(COLOR_WHITE);
        display.setTextSize(1);
        
        // Title
        display.printCentered(2, "SRDriver", 1);
        
        // Draw a separator line
        display.drawLine(0, 12, 128, 12, COLOR_WHITE);
        
        // Status info
        display.printAt(2, 20, "Status: Running", 1);
        
        // Show frame count
        char frameText[32];
        snprintf(frameText, sizeof(frameText), "Frames: %d", _frameCount);
        display.printAt(2, 30, frameText, 1);
        
        // Show uptime
        uint32_t uptime = millis() / 1000;  // Convert to seconds
        char uptimeText[32];
        snprintf(uptimeText, sizeof(uptimeText), "Uptime: %ds", uptime);
        display.printAt(2, 40, uptimeText, 1);
        
        // Draw a simple animation - bouncing dot
        uint8_t dotX = 64 + 30 * sin(_frameCount * 0.1);
        uint8_t dotY = 50 + 10 * cos(_frameCount * 0.15);
        display.fillCircle(dotX, dotY, 2, COLOR_WHITE);
        
        // Show the display
        display.show();
    }
}; 