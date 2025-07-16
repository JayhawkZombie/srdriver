#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "../hal/SSD_1306Component.h"
#include "../hal/display/DisplayQueue.h"
#include "../hal/display/DisplayBuffer.h"
#include <math.h>

// Forward declarations
extern SSD1306_Display display;

/**
 * DisplayTask - FreeRTOS task for OLED display management
 * 
 * Handles:
 * - Buffer-based display updates
 * - Banner message rendering
 * - Main display area rendering from DisplayQueue
 * - Hardware upload to physical display
 */
class DisplayTask : public SRTask {
public:
    DisplayTask(uint32_t updateIntervalMs = 33,  // 30 FPS for smooth updates
                uint32_t stackSize = 4096,
                UBaseType_t priority = tskIDLE_PRIORITY + 2,  // Medium priority
                BaseType_t core = 0)  // Pin to core 0
        : SRTask("DisplayTask", stackSize, priority, core),
          _display(display),
          _displayQueue(DisplayQueue::getInstance()),
          _updateInterval(updateIntervalMs),
          _frameCount(0) {}
    
    /**
     * Get current frame count
     */
    uint32_t getFrameCount() const { return _frameCount; }
    
    /**
     * Get update interval
     */
    uint32_t getUpdateInterval() const { return _updateInterval; }

protected:
    /**
     * Main task loop - handles display updates and rendering
     */
    void run() override;

private:
    SSD1306_Display& _display;
    DisplayQueue& _displayQueue;
    uint32_t _updateInterval;
    uint32_t _frameCount;
    
    /**
     * Update display content using buffer-based rendering
     */
    void updateDisplay();
    
    /**
     * Render the banner (yellow region)
     */
    void renderBanner();
    
    /**
     * Render main display area from buffer
     */
    void renderMainDisplay();
    
    /**
     * Upload buffer to hardware display
     */
    void uploadBufferToDisplay(const DisplayBuffer& buffer);
}; 