#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "../hal/display/SSD_1306Component.h"
#include "../hal/display/DisplayQueue.h"
#include "SystemMonitorTask.h"

// Forward declarations
extern SSD1306_Display display;
class SystemMonitorTask;  // Forward declaration only
extern SystemMonitorTask* g_systemMonitorTask;  // Defined in main.cpp

/**
 * OLEDDisplayTask - FreeRTOS task for OLED display management
 * 
 * Handles:
 * - Banner message rendering via DisplayQueue
 * - Default content rendering (firmware version, etc.)
 * 
 * This is OLED-specific. For other display types (e.g., LVGL on CrowPanel),
 * create a separate display task class.
 */
class OLEDDisplayTask : public SRTask {
public:
    OLEDDisplayTask(uint32_t updateIntervalMs = 200,  // 5 FPS for display updates
                    uint32_t stackSize = 4096,
                    UBaseType_t priority = tskIDLE_PRIORITY + 2,  // Medium priority
                    BaseType_t core = 0)  // Pin to core 0
        : SRTask("OLEDDisplay", stackSize, priority, core),
          _display(display),
          _displayQueue(DisplayQueue::getInstance()),
          _updateInterval(updateIntervalMs),
          _frameCount(0),
          _viewSwitchInterval(5000),  // Switch views every 5 seconds
          _lastViewSwitch(0),
          _showStats(false) {}
    
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
     * Update display content
     */
    void updateDisplay();
    
    /**
     * Render the banner (yellow region at top)
     */
    void renderBanner();
    
    /**
     * Render default content (firmware version, build date, etc.)
     */
    void renderDefaultContent();
    
    /**
     * Render system statistics (uptime, tasks, heap, etc.)
     */
    void renderSystemStats();
    
    // View switching state
    uint32_t _viewSwitchInterval;  // How often to switch views (ms)
    uint32_t _lastViewSwitch;       // Last time we switched views
    bool _showStats;                // true = show stats, false = show default
};

