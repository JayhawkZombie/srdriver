#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "PlatformConfig.h"
#include "SystemMonitorTask.h"

// Forward declarations
class TaskManager;
class JsonSettings;

#if PLATFORM_CROW_PANEL
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <esp_heap_caps.h>

/**
 * LVGLDisplayTask - FreeRTOS task for LVGL display management on CrowPanel
 * 
 * Handles:
 * - LVGL initialization and display setup
 * - System statistics display (uptime, heap, tasks, CPU, temperature/power)
 * - LVGL timer handling
 * 
 * This is LVGL-specific for CrowPanel. For OLED displays, use OLEDDisplayTask.
 */
class LVGLDisplayTask : public SRTask {
public:
    LVGLDisplayTask(const JsonSettings* settings = nullptr,
                    uint32_t updateIntervalMs = 50,  // 20 FPS for LVGL updates
                    uint32_t stackSize = 8192,       // LVGL needs more stack
                    UBaseType_t priority = tskIDLE_PRIORITY + 2,  // Medium priority
                    BaseType_t core = 0);  // Pin to core 0
    
    ~LVGLDisplayTask();
    
    /**
     * Get current frame count
     */
    uint32_t getFrameCount() const { return _frameCount; }
    
    /**
     * Get update interval
     */
    uint32_t getUpdateInterval() const { return _updateInterval; }
    
    /**
     * Initialize display hardware - must be called from setup() before task starts
     * Returns true if successful
     */
    bool initializeHardware();

protected:
    /**
     * Main task loop - handles LVGL updates and rendering
     */
    void run() override;

private:
    // Display hardware
    class LGFX : public lgfx::LGFX_Device {
    public:
        lgfx::Bus_RGB     _bus_instance;
        lgfx::Panel_RGB   _panel_instance;
        
        LGFX();
    };
    
    LGFX _lcd;
    uint32_t _screenWidth;
    uint32_t _screenHeight;
    const uint32_t _bufferLines = 80;  // Number of lines per draw buffer
    
    // LVGL display buffers
    lv_disp_draw_buf_t _drawBuf;
    lv_color_t* _buf1;
    lv_color_t* _buf2;
    lv_disp_drv_t _dispDrv;
    
    // LVGL tick timer
    hw_timer_t* _lvglTickTimer;
    
    // LVGL UI objects
    lv_obj_t* _screen;
    lv_obj_t* _uptimeLabel;
    lv_obj_t* _heapLabel;
    lv_obj_t* _tasksLabel;
    lv_obj_t* _cpuLabel;
    lv_obj_t* _tempPowerLabel;
    
    // Task state
    uint32_t _updateInterval;
    uint32_t _frameCount;
    uint32_t _lastStatsUpdate;
    static constexpr uint32_t STATS_UPDATE_INTERVAL_MS = 1000;  // Update stats every second
    
    /**
     * Initialize LVGL display hardware and buffers
     */
    bool initializeDisplay();
    
    /**
     * Initialize LVGL tick timer (1ms)
     */
    bool initializeTickTimer();
    
    /**
     * Create the UI screen with labels
     */
    void createUI();
    
    /**
     * Update system statistics labels
     */
    void updateSystemStats();
    
    /**
     * Display flush callback for LVGL
     */
    static void displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    
    /**
     * Touchpad read callback (dummy for now)
     */
    static void touchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
    
    /**
     * LVGL tick callback (IRAM_ATTR)
     */
    static void IRAM_ATTR lvglTickCallback();
};
#endif // PLATFORM_CROW_PANEL

