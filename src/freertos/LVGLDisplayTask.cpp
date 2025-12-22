#include "LVGLDisplayTask.h"
#include "TaskManager.h"
#include "../utility/StringUtils.h"

#if PLATFORM_CROW_PANEL

// Backlight pin for CrowPanel
#ifndef TFT_BL
#define TFT_BL 2
#endif

// LVGL tick callback - must be in IRAM
void IRAM_ATTR LVGLDisplayTask::lvglTickCallback() {
    lv_tick_inc(1);
}

// Display flush callback
void LVGLDisplayTask::displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    LVGLDisplayTask* task = static_cast<LVGLDisplayTask*>(disp->user_data);
    if (!task) {
        lv_disp_flush_ready(disp);
        return;
    }
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    task->_lcd.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t *) &color_p->full);
    lv_disp_flush_ready(disp);
}

// Touchpad read callback (dummy for now)
void LVGLDisplayTask::touchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    // No touch input for now - just report no touch
    data->state = LV_INDEV_STATE_REL;
}

// LGFX constructor - configure CrowPanel RGB display
LVGLDisplayTask::LGFX::LGFX() {
    {
        auto cfg = _bus_instance.config();
        cfg.panel = &_panel_instance;
        
        // RGB data pins
        cfg.pin_d0 = GPIO_NUM_15;  // B0
        cfg.pin_d1 = GPIO_NUM_7;   // B1
        cfg.pin_d2 = GPIO_NUM_6;   // B2
        cfg.pin_d3 = GPIO_NUM_5;   // B3
        cfg.pin_d4 = GPIO_NUM_4;   // B4
        cfg.pin_d5 = GPIO_NUM_9;   // G0
        cfg.pin_d6 = GPIO_NUM_46;  // G1
        cfg.pin_d7 = GPIO_NUM_3;   // G2
        cfg.pin_d8 = GPIO_NUM_8;   // G3
        cfg.pin_d9 = GPIO_NUM_16;  // G4
        cfg.pin_d10 = GPIO_NUM_1;  // G5
        cfg.pin_d11 = GPIO_NUM_14; // R0
        cfg.pin_d12 = GPIO_NUM_21; // R1
        cfg.pin_d13 = GPIO_NUM_47; // R2
        cfg.pin_d14 = GPIO_NUM_48; // R3
        cfg.pin_d15 = GPIO_NUM_45; // R4
        
        // Control pins
        cfg.pin_henable = GPIO_NUM_41;
        cfg.pin_vsync = GPIO_NUM_40;
        cfg.pin_hsync = GPIO_NUM_39;
        cfg.pin_pclk = GPIO_NUM_0;
        cfg.freq_write = 15000000;
        
        // Timing configuration
        cfg.hsync_polarity = 0;
        cfg.hsync_front_porch = 40;
        cfg.hsync_pulse_width = 48;
        cfg.hsync_back_porch = 40;
        cfg.vsync_polarity = 0;
        cfg.vsync_front_porch = 1;
        cfg.vsync_pulse_width = 31;
        cfg.vsync_back_porch = 13;
        cfg.pclk_active_neg = 1;
        cfg.de_idle_high = 0;
        cfg.pclk_idle_high = 0;
        
        _bus_instance.config(cfg);
    }
    
    {
        auto cfg = _panel_instance.config();
        cfg.memory_width = 800;
        cfg.memory_height = 480;
        cfg.panel_width = 800;
        cfg.panel_height = 480;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        _panel_instance.config(cfg);
    }
    
    _panel_instance.setBus(&_bus_instance);
    _panel_instance.setBrightness(255);
    setPanel(&_panel_instance);
}

LVGLDisplayTask::LVGLDisplayTask(const JsonSettings* settings,
                                 uint32_t updateIntervalMs,
                                 uint32_t stackSize,
                                 UBaseType_t priority,
                                 BaseType_t core)
    : SRTask("LVGLDisplay", stackSize, priority, core),
      _lcd(),
      _screenWidth(800),
      _screenHeight(480),
      _buf1(nullptr),
      _buf2(nullptr),
      _lvglTickTimer(nullptr),
      _screen(nullptr),
      _uptimeLabel(nullptr),
      _heapLabel(nullptr),
      _tasksLabel(nullptr),
      _cpuLabel(nullptr),
      _tempPowerLabel(nullptr),
      _updateInterval(updateIntervalMs),
      _frameCount(0),
      _lastStatsUpdate(0)
{
    // Initialize display buffers to nullptr - will be allocated in initializeDisplay()
}

LVGLDisplayTask::~LVGLDisplayTask() {
    // Clean up LVGL tick timer
    if (_lvglTickTimer) {
        timerEnd(_lvglTickTimer);
        _lvglTickTimer = nullptr;
    }
    
    // Free display buffers
    if (_buf1) {
        heap_caps_free(_buf1);
        _buf1 = nullptr;
    }
    if (_buf2) {
        heap_caps_free(_buf2);
        _buf2 = nullptr;
    }
}

bool LVGLDisplayTask::initializeHardware() {
    // Initialize LovyanGFX display hardware - must be called from setup()
    LOG_INFO_COMPONENT("LVGLDisplay", "Initializing display hardware...");
    _lcd.begin();
    _lcd.fillScreen(TFT_BLACK);
    delay(200);
    
    // Get actual screen dimensions
    _screenWidth = _lcd.width();
    _screenHeight = _lcd.height();
    LOG_INFOF_COMPONENT("LVGLDisplay", "Display initialized: %dx%d", _screenWidth, _screenHeight);
    return true;
}

bool LVGLDisplayTask::initializeDisplay() {
    // Check if hardware was initialized
    if (_screenWidth == 0 || _screenHeight == 0) {
        LOG_ERROR_COMPONENT("LVGLDisplay", "Display hardware not initialized - call initializeHardware() first");
        return false;
    }
    
    // Initialize LVGL
    lv_init();
    
    // Allocate display buffers in PSRAM for double buffering
    size_t buf_pixels = static_cast<size_t>(_screenWidth) * _bufferLines;
    _buf1 = (lv_color_t *) heap_caps_malloc(buf_pixels * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    _buf2 = (lv_color_t *) heap_caps_malloc(buf_pixels * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    
    // If PSRAM DMA allocation fails, fall back to smaller internal buffer
    if (!_buf1 || !_buf2) {
        LOG_WARN_COMPONENT("LVGLDisplay", "PSRAM allocation failed, using smaller internal buffer");
        buf_pixels = static_cast<size_t>(_screenWidth) * 40;  // Smaller fallback
        _buf1 = (lv_color_t *) heap_caps_malloc(buf_pixels * sizeof(lv_color_t),
            MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        _buf2 = nullptr;
        if (!_buf1) {
            LOG_ERROR_COMPONENT("LVGLDisplay", "Failed to allocate display buffers");
            return false;
        }
    }
    
    // Initialize LVGL display buffer
    lv_disp_draw_buf_init(&_drawBuf, _buf1, _buf2, buf_pixels);
    
    // Initialize and register display driver
    lv_disp_drv_init(&_dispDrv);
    _dispDrv.hor_res = _screenWidth;
    _dispDrv.ver_res = _screenHeight;
    _dispDrv.flush_cb = displayFlush;
    _dispDrv.draw_buf = &_drawBuf;
    _dispDrv.user_data = this;  // Store task pointer for flush callback
    lv_disp_drv_register(&_dispDrv);
    
    // Initialize dummy input device (touch - not implemented yet)
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpadRead;
    lv_indev_drv_register(&indev_drv);
    
    // Set up backlight (if defined)
    #ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);
    delay(500);
    digitalWrite(TFT_BL, HIGH);
    
    // PWM backlight control
    ledcSetup(1, 300, 8);
    ledcAttachPin(TFT_BL, 1);
    ledcWrite(1, 255);  // Full brightness
    #endif
    
    LOG_INFO_COMPONENT("LVGLDisplay", "Display initialized");
    return true;
}

bool LVGLDisplayTask::initializeTickTimer() {
    // Set up 1ms LVGL tick timer
    _lvglTickTimer = timerBegin(0, 80, true);  // 80 MHz / 80 = 1 MHz
    if (!_lvglTickTimer) {
        LOG_ERROR_COMPONENT("LVGLDisplay", "Failed to create LVGL tick timer");
        return false;
    }
    
    timerAttachInterrupt(_lvglTickTimer, &lvglTickCallback, true);
    timerAlarmWrite(_lvglTickTimer, 1000, true);  // 1000 µs = 1 ms
    timerAlarmEnable(_lvglTickTimer);
    
    LOG_INFO_COMPONENT("LVGLDisplay", "LVGL tick timer initialized");
    return true;
}

void LVGLDisplayTask::createUI() {
    // Create a simple black screen
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_scr_load(_screen);
    
    // Create labels for system stats
    // Uptime label (top, large)
    _uptimeLabel = lv_label_create(_screen);
    lv_obj_set_style_text_color(_uptimeLabel, lv_color_white(), 0);
    // lv_obj_set_style_text_font(_uptimeLabel, &lv_font_montserrat_24, 0);
    lv_obj_align(_uptimeLabel, LV_ALIGN_TOP_MID, 0, 20);
    lv_label_set_text(_uptimeLabel, "Uptime: 0d 0h 0m 0s");
    
    // Heap label
    _heapLabel = lv_label_create(_screen);
    lv_obj_set_style_text_color(_heapLabel, lv_color_white(), 0);
    // lv_obj_set_style_text_font(_heapLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(_heapLabel, LV_ALIGN_TOP_MID, 0, 80);
    lv_label_set_text(_heapLabel, "Heap: 0% (0KB)");
    
    // Tasks label
    _tasksLabel = lv_label_create(_screen);
    lv_obj_set_style_text_color(_tasksLabel, lv_color_white(), 0);
    // lv_obj_set_style_text_font(_tasksLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(_tasksLabel, LV_ALIGN_TOP_MID, 0, 120);
    lv_label_set_text(_tasksLabel, "Tasks: 0");
    
    // CPU label
    _cpuLabel = lv_label_create(_screen);
    lv_obj_set_style_text_color(_cpuLabel, lv_color_white(), 0);
    // lv_obj_set_style_text_font(_cpuLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(_cpuLabel, LV_ALIGN_TOP_MID, 0, 160);
    lv_label_set_text(_cpuLabel, "CPU: 0 MHz");
    
    // Temperature/Power label
    _tempPowerLabel = lv_label_create(_screen);
    lv_obj_set_style_text_color(_tempPowerLabel, lv_color_white(), 0);
    // lv_obj_set_style_text_font(_tempPowerLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(_tempPowerLabel, LV_ALIGN_TOP_MID, 0, 200);
    lv_label_set_text(_tempPowerLabel, "---");
    
    LOG_INFO_COMPONENT("LVGLDisplay", "UI created");
}

void LVGLDisplayTask::updateSystemStats() {
    auto* sysMon = TaskManager::getInstance().getSystemMonitorTask();
    if (!sysMon) {
        return;
    }

    LOG_DEBUGF_COMPONENT("LVGLDisplay", "Updating system stats");
    
    // Get system stats
    SystemStats stats = sysMon->getStats();
    
    // Update uptime label
    uint32_t uptime = stats.uptimeSeconds;
    uint32_t days = uptime / 86400;
    uint32_t hours = (uptime % 86400) / 3600;
    uint32_t minutes = (uptime % 3600) / 60;
    uint32_t seconds = uptime % 60;
    
    char uptimeText[64];
    snprintf(uptimeText, sizeof(uptimeText), "Uptime: %u d %u h %u m %u s",
             (unsigned int)days, (unsigned int)hours, 
             (unsigned int)minutes, (unsigned int)seconds);
    lv_label_set_text(_uptimeLabel, uptimeText);
    
    // Update heap label
    char heapText[64];
    snprintf(heapText, sizeof(heapText), "Heap: %d%% (%d KB free)",
             stats.heapUsagePercent, stats.freeHeap / 1024);
    lv_label_set_text(_heapLabel, heapText);
    
    // Update tasks label
    char tasksText[64];
    snprintf(tasksText, sizeof(tasksText), "Tasks: %d", stats.taskCount);
    lv_label_set_text(_tasksLabel, tasksText);
    
    // Update CPU label
    char cpuText[64];
    snprintf(cpuText, sizeof(cpuText), "CPU: %d MHz", stats.cpuFreqMHz);
    lv_label_set_text(_cpuLabel, cpuText);
    
    // Update temperature/power label
    char tempPowerText[64];
    if (stats.temperatureAvailable) {
        snprintf(tempPowerText, sizeof(tempPowerText), "Temp: %.0f°F", stats.temperatureF);
    } else if (stats.powerAvailable) {
        snprintf(tempPowerText, sizeof(tempPowerText), "Power: %.1f W", stats.power_W);
    } else {
        snprintf(tempPowerText, sizeof(tempPowerText), "---");
    }
    lv_label_set_text(_tempPowerLabel, tempPowerText);
}

void LVGLDisplayTask::run() {
    LOG_INFO_COMPONENT("LVGLDisplay", "LVGL display task started");
    LOG_INFOF_COMPONENT("LVGLDisplay", "Update interval: %d ms (~%d FPS)", 
               _updateInterval, 1000 / _updateInterval);
    
    // Initialize display hardware
    if (!initializeDisplay()) {
        LOG_ERROR_COMPONENT("LVGLDisplay", "Failed to initialize display");
        return;
    }
    
    // Initialize LVGL tick timer
    if (!initializeTickTimer()) {
        LOG_ERROR_COMPONENT("LVGLDisplay", "Failed to initialize tick timer");
        return;
    }
    
    // Create UI
    createUI();
    
    // Initial stats update
    updateSystemStats();
    lv_timer_handler();  // Render initial frame
    
    LOG_INFO_COMPONENT("LVGLDisplay", "LVGL display ready");
    
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Update system stats periodically
        uint32_t now = millis();
        if (now - _lastStatsUpdate >= STATS_UPDATE_INTERVAL_MS) {
            updateSystemStats();
            _lastStatsUpdate = now;
        }
        
        // Handle LVGL timers and rendering
        lv_timer_handler();
        
        // Increment frame counter
        _frameCount++;
        
        // Log status every 10 seconds
        static uint32_t lastLogTime = 0;
        if (now - lastLogTime > 10000) {
            LOG_DEBUGF_COMPONENT("LVGLDisplay", "Display Update - Frames: %d, Interval: %d ms", 
                      _frameCount, _updateInterval);
            _frameCount = 0;
            lastLogTime = now;
        }
        
        // Sleep until next update
        SRTask::sleepUntil(&lastWakeTime, _updateInterval);
    }
}

#endif // PLATFORM_CROW_PANEL

