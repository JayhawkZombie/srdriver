#pragma once

#include "SSD_1306Component.h"
#include <vector>

/**
 * DisplayRenderer - High-level drawing abstraction
 * 
 * Provides:
 * - Layout management (grid, flex, absolute positioning)
 * - Common UI patterns (cards, lists, dialogs)
 * - Text formatting and wrapping
 * - Animation helpers
 * - Drawing state management
 */
class DisplayRenderer {
public:
    DisplayRenderer(SSD1306_Display& display);
    
    // Layout management
    struct Layout {
        int x, y, width, height;
        int padding;
        int margin;
        bool centered;
        
        Layout(int x = 0, int y = 0, int w = 128, int h = 64) 
            : x(x), y(y), width(w), height(h), padding(2), margin(1), centered(false) {}
    };
    
    // Drawing state
    void saveState();
    void restoreState();
    void setTextStyle(uint8_t size);
    
    // Layout helpers
    Layout createLayout(int x, int y, int width, int height);
    Layout createCenteredLayout(int width, int height);
    Layout createGridLayout(int rows, int cols, int cellWidth, int cellHeight);
    
    // High-level drawing functions
    void drawCard(const Layout& layout, const String& title, const String& content);
    void drawList(const Layout& layout, const std::vector<String>& items, int selectedIndex = -1);
    void drawDialog(const Layout& layout, const String& title, const String& message, const std::vector<String>& buttons);
    void drawStatusBar(const String& leftText, const String& rightText);
    void drawProgressBar(const Layout& layout, uint8_t percent);
    void drawProgressCard(const Layout& layout, const String& title, uint8_t progress, const String& status = "");
    void drawInfoCard(const Layout& layout, const String& title, const std::vector<String>& info);
    
    // Text helpers
    void drawWrappedText(const Layout& layout, const String& text, uint8_t lineHeight = 10);
    void drawCenteredText(const Layout& layout, const String& text);
    void drawRightAlignedText(const Layout& layout, const String& text);
    int getTextWidth(const String& text, uint8_t size = 1);
    int getTextHeight(const String& text, uint8_t size = 1, int maxWidth = 128);
    
    // Animation helpers
    void drawAnimatedProgress(const Layout& layout, uint8_t progress, uint32_t frame);
    void drawLoadingSpinner(const Layout& layout, uint32_t frame);
    void drawPulseEffect(const Layout& layout, uint32_t frame);
    void drawFadeIn(const Layout& layout, uint32_t frame, uint32_t duration = 30);
    void drawSlideIn(const Layout& layout, uint32_t frame, uint32_t duration = 30, const String& direction = "left");
    
    // Icon drawing
    void drawIcon(const Layout& layout, const String& iconType);
    void drawBatteryIcon(int x, int y, uint8_t level);
    void drawWiFiIcon(int x, int y, uint8_t strength);
    void drawBluetoothIcon(int x, int y, bool connected);
    void drawArrowIcon(int x, int y, const String& direction);
    
    // Chart helpers
    void drawLineChart(const Layout& layout, const std::vector<uint8_t>& data, bool showGrid = true);
    void drawBarChart(const Layout& layout, const std::vector<uint8_t>& data, bool horizontal = false);
    void drawPieChart(const Layout& layout, const std::vector<uint8_t>& data, const std::vector<String>& labels);
    
    // Game helpers
    void drawGameGrid(const Layout& layout, int rows, int cols, const std::vector<std::vector<bool>>& grid);
    void drawSprite(const Layout& layout, const std::vector<std::vector<bool>>& sprite);
    void drawParticle(const Layout& layout, int x, int y, uint8_t size, uint32_t frame);
    
    // Utility
    void clearArea(const Layout& layout);
    void drawBorder(const Layout& layout);
    void drawBackground(const Layout& layout);
    bool isPointInLayout(const Layout& layout, int x, int y);
    
    // Get underlying display
    SSD1306_Display& getDisplay() { return _display; }

private:
    SSD1306_Display& _display;
    
    // Drawing state
    struct DrawState {
        uint8_t textSize;
    };
    std::vector<DrawState> _stateStack;
    
    // Helper functions
    void drawGrid(const Layout& layout, int rows, int cols);
    void drawIconInternal(int x, int y, const String& type);
    int wrapText(const String& text, int maxWidth, std::vector<String>& lines);
    void drawAnimatedBar(const Layout& layout, uint8_t progress, uint32_t frame, bool vertical = false);
};

/**
 * DisplayTheme - Consistent styling
 */
class DisplayTheme {
public:
    DisplayTheme();
    
    // Typography
    uint8_t titleSize;
    uint8_t bodySize;
    uint8_t captionSize;
    
    // Spacing
    int padding;
    int margin;
    int borderRadius;
    
    // Animation
    uint32_t animationSpeed;
    uint32_t transitionDuration;
    
    // Predefined themes
    static DisplayTheme getDefaultTheme();
    static DisplayTheme getMinimalTheme();
    static DisplayTheme getCompactTheme();
};

/**
 * DisplayAnimation - Animation framework
 */
class DisplayAnimation {
public:
    enum class Type {
        FADE_IN,
        FADE_OUT,
        SLIDE_LEFT,
        SLIDE_RIGHT,
        SLIDE_UP,
        SLIDE_DOWN,
        SCALE_IN,
        SCALE_OUT,
        ROTATE,
        PULSE,
        BOUNCE
    };
    
    DisplayAnimation(Type type, uint32_t duration = 1000);
    
    void start();
    void stop();
    void pause();
    void resume();
    
    bool isRunning() const { return _running; }
    bool isFinished() const { return _finished; }
    float getProgress() const;
    
    // Animation values
    float getAlpha() const;
    float getScale() const;
    float getRotation() const;
    int getOffsetX() const;
    int getOffsetY() const;
    
    // Easing functions
    float easeInOut(float t) const;
    float easeIn(float t) const;
    float easeOut(float t) const;
    float bounce(float t) const;
    
private:
    Type _type;
    uint32_t _duration;
    uint32_t _startTime;
    uint32_t _pauseTime;
    bool _running;
    bool _finished;
    bool _paused;
    
    uint32_t getElapsedTime() const;
}; 