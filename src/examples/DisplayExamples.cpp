#include "../hal/SSD_1306Component.h"

// Example functions to demonstrate display capabilities
// These can be called from your main code or other tasks

/**
 * Example 1: Basic text display
 */
void exampleBasicText(SSD1306_Display& display) {
    display.clear();
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(2, "Basic Text", 1);
    
    // Some text lines
    display.printAt(2, 20, "Line 1: Hello World", 1);
    display.printAt(2, 30, "Line 2: SRDriver", 1);
    display.printAt(2, 40, "Line 3: OLED Test", 1);
    
    display.show();
}

/**
 * Example 2: Progress bar
 */
void exampleProgressBar(SSD1306_Display& display, uint8_t percent) {
    display.clear();
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(2, "Progress Bar", 1);
    
    // Progress bar
    display.drawProgressBar(10, 20, 108, 12, percent);
    
    // Percentage text
    char percentText[16];
    snprintf(percentText, sizeof(percentText), "%d%%", percent);
    display.printCentered(40, percentText, 1);
    
    display.show();
}

/**
 * Example 3: Simple graph
 */
void exampleBarGraph(SSD1306_Display& display) {
    display.clear();
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(2, "Bar Graph", 1);
    
    // Sample data
    uint8_t data[] = {20, 45, 70, 35, 80, 60, 30, 55};
    display.drawBarGraph(10, 20, 100, 30, data, 8);
    
    display.show();
}

/**
 * Example 4: Shapes and patterns
 */
void exampleShapes(SSD1306_Display& display) {
    display.clear();
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(2, "Shapes Demo", 1);
    
    // Draw various shapes
    display.drawRect(10, 20, 30, 20, COLOR_WHITE);      // Rectangle
    display.fillCircle(60, 30, 10, COLOR_WHITE);        // Filled circle
    display.drawTriangle(100, 20, 110, 40, 90, 40, COLOR_WHITE); // Triangle
    
    // Lines
    display.drawLine(10, 50, 118, 50, COLOR_WHITE);     // Horizontal line
    display.drawLine(64, 50, 64, 60, COLOR_WHITE);      // Vertical line
    
    display.show();
}

/**
 * Example 5: Animated bouncing ball
 */
void exampleBouncingBall(SSD1306_Display& display, uint32_t frame) {
    display.clear();
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(2, "Bouncing Ball", 1);
    
    // Calculate ball position
    uint8_t ballX = 64 + 30 * sin(frame * 0.1);
    uint8_t ballY = 32 + 15 * cos(frame * 0.15);
    
    // Draw ball
    display.fillCircle(ballX, ballY, 4, COLOR_WHITE);
    
    // Draw trail
    for (int i = 1; i <= 3; i++) {
        uint8_t trailX = 64 + 30 * sin((frame - i * 5) * 0.1);
        uint8_t trailY = 32 + 15 * cos((frame - i * 5) * 0.15);
        display.drawCircle(trailX, trailY, 2, COLOR_WHITE);
    }
    
    display.show();
}

/**
 * Example 6: System status display
 */
void exampleSystemStatus(SSD1306_Display& display, uint32_t uptime, uint8_t brightness, const char* pattern) {
    display.clear();
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(2, "System Status", 1);
    
    // Draw separator
    display.drawLine(0, 12, 128, 12, COLOR_WHITE);
    
    // System info
    char uptimeText[32];
    snprintf(uptimeText, sizeof(uptimeText), "Uptime: %ds", uptime);
    display.printAt(2, 20, uptimeText, 1);
    
    char brightnessText[32];
    snprintf(brightnessText, sizeof(brightnessText), "Bright: %d%%", brightness);
    display.printAt(2, 30, brightnessText, 1);
    
    display.printAt(2, 40, "Pattern:", 1);
    display.printAt(50, 40, pattern, 1);
    
    // Draw a small status indicator
    display.fillCircle(120, 50, 3, COLOR_WHITE);
    
    display.show();
}

/**
 * Example 7: Menu-style display
 */
void exampleMenu(SSD1306_Display& display, uint8_t selectedItem) {
    display.clear();
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(2, "Menu", 1);
    
    // Menu items
    const char* items[] = {"Patterns", "Settings", "System", "About"};
    const int numItems = 4;
    
    for (int i = 0; i < numItems; i++) {
        int y = 20 + i * 10;
        
        // Draw selection indicator
        if (i == selectedItem) {
            display.fillRect(2, y-1, 4, 8, COLOR_WHITE);
        }
        
        // Draw menu text
        display.printAt(10, y, items[i], 1);
    }
    
    display.show();
}

/**
 * Example 8: Loading animation
 */
void exampleLoading(SSD1306_Display& display, uint32_t frame) {
    display.clear();
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    
    // Title
    display.printCentered(2, "Loading...", 1);
    
    // Animated dots
    uint8_t dotPos = (frame / 10) % 4;
    for (int i = 0; i < 4; i++) {
        int x = 50 + i * 8;
        int y = 30;
        
        if (i == dotPos) {
            display.fillCircle(x, y, 2, COLOR_WHITE);
        } else {
            display.drawCircle(x, y, 2, COLOR_WHITE);
        }
    }
    
    // Progress bar
    uint8_t progress = (frame % 100);
    display.drawProgressBar(10, 45, 108, 8, progress);
    
    display.show();
} 