#pragma once

#include "SSD_1306Component.h"
#include <vector>
#include <functional>
#include <memory>

// Forward declarations
class DisplayScreen;
class DisplayWidget;

/**
 * DisplayManager - High-level abstraction for managing display content
 * 
 * Provides:
 * - Screen management and transitions
 * - Widget system for reusable UI components
 * - Layout management
 * - Animation framework
 * - Event system
 */
class DisplayManager {
public:
    DisplayManager(SSD1306_Display& display);
    ~DisplayManager();
    
    // Screen management
    void addScreen(std::shared_ptr<DisplayScreen> screen);
    void showScreen(const String& screenName);
    void showScreen(int screenIndex);
    void nextScreen();
    void previousScreen();
    
    // Widget management
    void addWidget(std::shared_ptr<DisplayWidget> widget);
    void removeWidget(const String& widgetName);
    
    // Animation control
    void setAnimationSpeed(uint32_t fps);
    void pauseAnimation();
    void resumeAnimation();
    
    // Update loop (call from FreeRTOS task)
    void update();
    
    // Utility
    SSD1306_Display& getDisplay() { return _display; }
    int getCurrentScreenIndex() const { return _currentScreenIndex; }
    const String& getCurrentScreenName() const;

private:
    SSD1306_Display& _display;
    std::vector<std::shared_ptr<DisplayScreen>> _screens;
    std::vector<std::shared_ptr<DisplayWidget>> _widgets;
    int _currentScreenIndex;
    uint32_t _frameCount;
    uint32_t _animationSpeed;
    bool _animationPaused;
    
    void drawCurrentScreen();
    void updateWidgets();
};

/**
 * DisplayWidget - Reusable UI component
 */
class DisplayWidget {
public:
    DisplayWidget(const String& name, int x, int y, int width, int height);
    virtual ~DisplayWidget() = default;
    
    // Core interface
    virtual void draw(SSD1306_Display& display) = 0;
    virtual void update(uint32_t frameCount) = 0;
    
    // Properties
    const String& getName() const { return _name; }
    void setPosition(int x, int y);
    void setSize(int width, int height);
    void setVisible(bool visible) { _visible = visible; }
    bool isVisible() const { return _visible; }
    
    // Event handling
    virtual void onTouch(int x, int y) {}  // For future touch support
    virtual void onValueChange(int value) {}

protected:
    String _name;
    int _x, _y, _width, _height;
    bool _visible;
};

/**
 * DisplayScreen - Container for display content
 */
class DisplayScreen {
public:
    DisplayScreen(const String& name);
    virtual ~DisplayScreen() = default;
    
    // Core interface
    virtual void draw(SSD1306_Display& display) = 0;
    virtual void update(uint32_t frameCount) = 0;
    
    // Lifecycle
    virtual void onShow() {}
    virtual void onHide() {}
    virtual void onEnter() {}
    virtual void onExit() {}
    
    // Properties
    const String& getName() const { return _name; }
    void setBackgroundColor(uint16_t color) { _backgroundColor = color; }
    
protected:
    String _name;
    uint16_t _backgroundColor;
};

// Pre-built widget implementations
class TextWidget : public DisplayWidget {
public:
    TextWidget(const String& name, int x, int y, const String& text, uint8_t size = 1);
    
    void draw(SSD1306_Display& display) override;
    void update(uint32_t frameCount) override;
    
    void setText(const String& text);
    void setTextSize(uint8_t size);
    void setCentered(bool centered);
    
private:
    String _text;
    uint8_t _textSize;
    bool _centered;
};

class ProgressBarWidget : public DisplayWidget {
public:
    ProgressBarWidget(const String& name, int x, int y, int width, int height);
    
    void draw(SSD1306_Display& display) override;
    void update(uint32_t frameCount) override;
    
    void setProgress(uint8_t percent);
    void setAnimated(bool animated);
    
private:
    uint8_t _progress;
    bool _animated;
    uint32_t _animationOffset;
};

class GraphWidget : public DisplayWidget {
public:
    GraphWidget(const String& name, int x, int y, int width, int height, int maxPoints = 20);
    
    void draw(SSD1306_Display& display) override;
    void update(uint32_t frameCount) override;
    
    void addDataPoint(uint8_t value);
    void clearData();
    void setAutoScale(bool autoScale);
    
private:
    std::vector<uint8_t> _dataPoints;
    int _maxPoints;
    bool _autoScale;
    uint8_t _maxValue;
};

class IconWidget : public DisplayWidget {
public:
    IconWidget(const String& name, int x, int y, const String& iconType);
    
    void draw(SSD1306_Display& display) override;
    void update(uint32_t frameCount) override;
    
    void setIconType(const String& iconType);
    void setAnimated(bool animated);
    
private:
    String _iconType;
    bool _animated;
    uint32_t _animationFrame;
    
    void drawIcon(SSD1306_Display& display, const String& type);
};

// Pre-built screen implementations
class StatusScreen : public DisplayScreen {
public:
    StatusScreen();
    
    void draw(SSD1306_Display& display) override;
    void update(uint32_t frameCount) override;
    
    void setStatus(const String& status);
    void setUptime(uint32_t uptime);
    void setBrightness(uint8_t brightness);
    
private:
    String _status;
    uint32_t _uptime;
    uint8_t _brightness;
    uint32_t _frameCount;
};

class MenuScreen : public DisplayScreen {
public:
    MenuScreen();
    
    void draw(SSD1306_Display& display) override;
    void update(uint32_t frameCount) override;
    
    void addMenuItem(const String& item);
    void setSelectedItem(int index);
    void nextItem();
    void previousItem();
    
private:
    std::vector<String> _menuItems;
    int _selectedItem;
    int _scrollOffset;
};

class AnimationScreen : public DisplayScreen {
public:
    AnimationScreen();
    
    void draw(SSD1306_Display& display) override;
    void update(uint32_t frameCount) override;
    
    void setAnimationType(const String& type);
    
private:
    String _animationType;
    uint32_t _frameCount;
    
    void drawAnimation(SSD1306_Display& display, const String& type);
}; 