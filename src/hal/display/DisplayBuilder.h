#pragma once

#include "SSD_1306Component.h"
#include <vector>
#include <functional>

/**
 * DisplayBuilder - Declarative UI builder pattern
 * 
 * Allows creating complex displays using a fluent interface:
 * 
 * DisplayBuilder(display)
 *   .card(10, 10, 108, 44)
 *     .title("Status")
 *     .text("System Running")
 *     .progress(75)
 *   .end()
 *   .icon(120, 10, "wifi")
 *   .text(2, 60, "Uptime: 1h 23m")
 *   .render();
 */
class DisplayBuilder {
public:
    DisplayBuilder(SSD1306_Display& display);
    
    // Container elements
    DisplayBuilder& card(int x, int y, int width, int height);
    DisplayBuilder& list(int x, int y, int width, int height);
    DisplayBuilder& grid(int x, int y, int rows, int cols, int cellWidth, int cellHeight);
    DisplayBuilder& dialog(int x, int y, int width, int height);
    
    // Content elements
    DisplayBuilder& title(const String& text);
    DisplayBuilder& text(const String& text);
    DisplayBuilder& text(int x, int y, const String& text);
    DisplayBuilder& progress(uint8_t percent);
    DisplayBuilder& icon(const String& type);
    DisplayBuilder& icon(int x, int y, const String& type);
    DisplayBuilder& button(const String& text);
    DisplayBuilder& separator();
    DisplayBuilder& statusBar(const String& leftText, const String& rightText);
    
    // Styling
    DisplayBuilder& size(uint8_t size);
    DisplayBuilder& centered();
    DisplayBuilder& rightAligned();
    DisplayBuilder& padding(int padding);
    DisplayBuilder& margin(int margin);
    
    // Animation
    DisplayBuilder& animate(const String& type, uint32_t duration = 1000);
    DisplayBuilder& fadeIn(uint32_t duration = 500);
    DisplayBuilder& slideIn(const String& direction = "left", uint32_t duration = 500);
    
    // Layout
    DisplayBuilder& row();
    DisplayBuilder& column();
    DisplayBuilder& space(int pixels);
    DisplayBuilder& flex();
    
    // Control
    DisplayBuilder& end();  // End current container
    DisplayBuilder& clear();  // Clear display
    void render();  // Render everything
    
    // Utility
    DisplayBuilder& debug(bool enabled = true);

private:
    SSD1306_Display& _display;
    
    // Current context
    struct Context {
        String type;  // "card", "list", "grid", etc.
        int x, y, width, height;
        uint8_t textSize;
        bool centered;
        bool rightAligned;
        int padding;
        int margin;
        String animation;
        uint32_t animationDuration;
    };
    
    std::vector<Context> _contextStack;
    Context _currentContext;
    bool _debugMode;
    
    // Helper methods
    void pushContext();
    void popContext();
    void applyContext();
    void drawElement(const String& type, const String& content);
    void log(const String& message);
};

/**
 * DisplayTemplate - Pre-built display templates
 */
class DisplayTemplate {
public:
    DisplayTemplate(SSD1306_Display& display);
    
    // Common templates
    void renderStatusScreen(const String& title, const String& status, uint8_t progress = 0);
    void renderMenuScreen(const String& title, const std::vector<String>& items, int selectedIndex = 0);
    void renderInfoScreen(const String& title, const std::vector<String>& info);
    void renderProgressScreen(const String& title, uint8_t progress, const String& status = "");
    void renderErrorScreen(const String& title, const String& error);
    void renderSuccessScreen(const String& title, const String& message);
    void renderLoadingScreen(const String& title, uint32_t frame);
    void renderGameScreen(const String& title, const std::vector<std::vector<bool>>& grid);
    void renderListScreen(const String& title, const std::vector<String>& items, int selectedIndex = 0);
    
    // Data visualization templates
    void renderChartScreen(const String& title, const std::vector<uint8_t>& data, const String& chartType = "bar");
    void renderGraphScreen(const String& title, const std::vector<uint8_t>& data, bool showGrid = true);
    void renderMetricScreen(const String& title, const String& value, const String& unit = "");
    
    // System templates
    void renderSystemStatus(const String& uptime, const String& memory, const String& cpu);
    void renderNetworkStatus(bool wifiConnected, bool bluetoothConnected, uint8_t batteryLevel);
    void renderSensorData(const String& sensorName, float value, const String& unit);
    
    // Animation templates
    void renderSplashScreen(const String& title, uint32_t frame);
    void renderTransitionScreen(const String& fromScreen, const String& toScreen, uint32_t frame);
    void renderNotificationScreen(const String& title, const String& message, uint32_t frame);

private:
    SSD1306_Display& _display;
    DisplayBuilder _builder;
    
    // Helper methods
    void drawBatteryIndicator(int x, int y, uint8_t level);
    void drawWiFiIndicator(int x, int y, bool connected);
    void drawBluetoothIndicator(int x, int y, bool connected);
    void drawNotificationBadge(int x, int y, bool hasNotification);
};

/**
 * DisplayPreset - Quick preset configurations
 */
class DisplayPreset {
public:
    DisplayPreset(SSD1306_Display& display);
    
    // Quick presets
    void minimal(const String& text);
    void centered(const String& text);
    void statusBar(const String& left, const String& right);
    void progressBar(uint8_t percent);
    void loadingSpinner();
    void errorMessage(const String& error);
    void successMessage(const String& message);
    void infoMessage(const String& title, const String& message);
    
    // Game presets
    void gameGrid(int rows, int cols, const std::vector<std::vector<bool>>& grid);
    void gameScore(int score, int highScore);
    void gameOver(int score, int highScore);
    
    // Data presets
    void sensorReading(const String& sensor, float value, const String& unit);
    void timeDisplay(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void dateDisplay(uint8_t day, uint8_t month, uint16_t year);
    void temperatureDisplay(float temp, const String& unit = "C");
    
    // System presets
    void systemInfo(const String& uptime, uint8_t memory, uint8_t cpu);
    void networkInfo(bool wifi, bool bluetooth, uint8_t battery);
    void storageInfo(uint8_t used, uint8_t total);

private:
    SSD1306_Display& _display;
    DisplayBuilder _builder;
    
    // Helper methods
    void drawIcon(int x, int y, const String& type);
    void drawProgress(int x, int y, int width, int height, uint8_t percent);
    void drawSpinner(int x, int y, uint32_t frame);
}; 