#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include "../hal/display/SSD_1306Component.h"
#include "../hal/display/DisplayQueue.h"
#include "SystemMonitorTask.h"

// Forward declarations
class SystemMonitorTask;  // Forward declaration only
class TaskManager;  // Forward declaration
class JsonSettings;  // Forward declaration

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
    OLEDDisplayTask(const JsonSettings* settings = nullptr,
                    uint32_t updateIntervalMs = 16,  // 5 FPS for display updates
                    uint32_t stackSize = 4096,
                    UBaseType_t priority = tskIDLE_PRIORITY + 2,  // Medium priority
                    BaseType_t core = 0);  // Pin to core 0
    
    /**
     * Get current frame count
     */
    uint32_t getFrameCount() const { return _frameCount; }
    
    /**
     * Get update interval
     */
    uint32_t getUpdateInterval() const { return _updateInterval; }

    enum DisplayView : int {
        DEFAULT_VIEW,
        SYSTEM_STATS_VIEW,
        CAPABILITIES_VIEW,


        NUM_VIEWS
    };

protected:
    /**
     * Main task loop - handles display updates and rendering
     */
    void run() override;

private:
    SSD1306_Display _display;  // Own the display instance
    DisplayQueue& _displayQueue;
    uint32_t _updateInterval;
    uint32_t _frameCount;
    
    /**
     * Update display content
     */
    DisplayView _currentView = DEFAULT_VIEW;
    void updateDisplay();
    void goToNextView();
    
    /**
     * Render the banner (yellow region at top)
     */
    void renderBanner();

    struct BannerLine {
        struct Point {
            int x = 0;
            int y = 0;
            Point(int _x, int _y) : x(_x), y(_y) {}
        };
        enum Direction {
            UP,
            RIGHT,
            DOWN,
            LEFT,
        };
        Point start;
        Point end;
        Direction direction = UP;

        BannerLine(Point _start, Point _end, Direction _direction) : start(_start), end(_end), direction(_direction) {}
    };

    class AnimatedBorderLine {
    public:
        BannerLine _line;
        int _fill = 0;
        bool _isFilling = true;
        int _advance = 2;

        AnimatedBorderLine(const BannerLine& line) : _line(line) {}

        void animate()
        {
            if (_isFilling)
            {
                _fill += _advance;
            }
            else
            {
                _fill -= _advance;
            }
        }

        // void render()
        // {
        //     renderBannerLine(_line, _fill, _isFilling);
        // }

        bool isFinished()
        {
            if (_isFilling)
            {
                return _fill >= 127;
            }
        }

        void reset()
        {
            _fill = 0;
            _isFilling = true;
        }
    };

    BannerLine _bannerLines[4] = {
        { {0, 0}, {127, 0}, BannerLine::Direction::RIGHT },
        { {127, 0}, {127, 63}, BannerLine::Direction::DOWN },
        { {127, 63}, {0, 63}, BannerLine::Direction::LEFT },
        { {0, 63}, {0, 0}, BannerLine::Direction::UP },
    };

    void renderBannerLine(const BannerLine& line, int fill, bool isFilling);
    
    /**
     * Render default content (firmware version, build date, etc.)
     */
    void renderDefaultContent();

    /**
     * Render border, but with some pizzaz and animation
     */
    void renderBorder();

    bool renderBorderFill();
    bool renderBorderUnfill();

    // void renderBorderLine(int side, int x0, int y0, int x1, int y1, int direction);
    /**
     * Render system statistics (uptime, tasks, heap, etc.)
     */
    void renderSystemStats();

    void renderCapabilities();
    
    // View switching state
    uint32_t _viewSwitchInterval;  // How often to switch views (ms)
    uint32_t _lastViewSwitch;       // Last time we switched views
    bool _showStats;                // true = show stats, false = show default
};

