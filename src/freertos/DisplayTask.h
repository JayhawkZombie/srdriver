#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "../hal/display/SSD_1306Component.h"
#include "../hal/display/DisplayQueue.h"
#include <math.h>

// Forward declarations
extern SSD1306_Display display;

// Function pointer type for render callbacks
typedef void (*DisplayRenderCallback)(SSD1306_Display& display);

/**
 * DisplayTask - FreeRTOS task for OLED display management
 * 
 * Handles:
 * - Display ownership management (only one task can render at a time)
 * - Default rendering when no task owns the display
 * - Banner message management via DisplayQueue
 * - Resource coordination between tasks
 */
class DisplayTask : public SRTask {
public:
    DisplayTask(uint32_t updateIntervalMs = 200,  // 5 FPS for display updates
                uint32_t stackSize = 4096,
                UBaseType_t priority = tskIDLE_PRIORITY + 2,  // Medium priority
                BaseType_t core = 0)  // Pin to core 0
        : SRTask("DisplayTask", stackSize, priority, core),
          _display(display),
          _displayQueue(DisplayQueue::getInstance()),
          _updateInterval(updateIntervalMs),
          _frameCount(0) {}
    
    /**
     * Request display ownership (static method for other tasks)
     * Returns true if ownership was granted, false if already owned
     */
    static bool requestOwnership(const String& taskName, DisplayRenderCallback renderCallback = nullptr);
    
    /**
     * Release display ownership (static method for other tasks)
     * Returns true if ownership was released, false if not owned by this task
     */
    static bool releaseOwnership(const String& taskName);
    
    /**
     * Check if display is currently owned
     */
    static bool isOwned() { return _currentOwner.length() > 0; }
    
    /**
     * Get current owner name
     */
    static String getCurrentOwner() { return _currentOwner; }
    
    /**
     * Get current frame count
     */
    uint32_t getFrameCount() const { return _frameCount; }
    
    /**
     * Get update interval
     */
    uint32_t getUpdateInterval() const { return _updateInterval; }
    
    /**
     * Get performance metrics
     */
    uint32_t getAverageFrameTime() const { return _averageFrameTime; }
    uint32_t getMaxFrameTime() const { return _maxFrameTime; }
    uint32_t getMissedFrames() const { return _missedFrames; }
    float getFrameRate() const { return _frameRate; }
    
    /**
     * Check if performance is acceptable
     */
    bool isPerformanceAcceptable() const;
    
    /**
     * Get performance report
     */
    String getPerformanceReport() const;

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
    
    // Performance monitoring
    uint32_t _lastFrameTime;
    uint32_t _averageFrameTime;
    uint32_t _maxFrameTime;
    uint32_t _missedFrames;
    float _frameRate;
    uint32_t _performanceSampleCount;
    
    // NEW: Ownership management
    static String _currentOwner;
    static DisplayRenderCallback _currentRenderCallback;
    
    /**
     * Update display content
     */
    void updateDisplay();
    
    /**
     * Render the banner (yellow region)
     */
    void renderBanner();
    
    /**
     * Render default content (animating ball) when no task owns the display
     */
    void renderDefaultContent();
    
    /**
     * Update performance metrics
     */
    void updatePerformanceMetrics(uint32_t frameTime);
}; 