#include "../hal/display/SSD_1306Component.h"

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

/**
 * Example 9: Pixel Art - Simple Robot Face
 */
void examplePixelArtRobot(SSD1306_Display& display) {
    display.clear();
    
    // Robot face - drawn with individual pixels using rectangles
    // Eyes
    display.fillRect(40, 20, 8, 8, COLOR_WHITE);
    display.fillRect(80, 20, 8, 8, COLOR_WHITE);
    
    // Nose
    display.fillRect(60, 30, 4, 4, COLOR_WHITE);
    
    // Mouth
    display.fillRect(50, 45, 28, 4, COLOR_WHITE);
    
    // Antenna
    display.fillRect(64, 10, 2, 8, COLOR_WHITE);
    display.fillCircle(65, 8, 2, COLOR_WHITE);
    
    // Title
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    display.printCentered(58, "Robot", 1);
    
    display.show();
}

/**
 * Example 10: Animated Fire Effect
 */
void exampleAnimatedFire(SSD1306_Display& display, uint32_t frame) {
    display.clear();
    
    // Simple fire effect using animated rectangles
    for (int i = 0; i < 8; i++) {
        int x = 20 + i * 12;
        int height = 15 + 8 * sin(frame * 0.1 + i * 0.5);
        int y = 50 - height;
        
        // Fire base
        display.fillRect(x, 50, 8, 14, COLOR_WHITE);
        
        // Fire tip
        display.fillRect(x+2, y, 4, 6, COLOR_WHITE);
    }
    
    // Title
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    display.printCentered(2, "Fire Effect", 1);
    
    display.show();
}

/**
 * Example 11: Digital Clock Display
 */
void exampleDigitalClock(SSD1306_Display& display, uint32_t frame) {
    display.clear();
    
    // Simulate time (in real use, you'd get actual time)
    uint32_t seconds = frame / 5;  // 5 frames per second
    uint8_t hours = (seconds / 3600) % 24;
    uint8_t minutes = (seconds / 60) % 60;
    uint8_t secs = seconds % 60;
    
    // Large time display
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(2);
    
    char timeText[16];
    snprintf(timeText, sizeof(timeText), "%02d:%02d", hours, minutes);
    display.printCentered(20, timeText, 2);
    
    // Seconds
    display.setTextSize(1);
    snprintf(timeText, sizeof(timeText), ":%02d", secs);
    display.printAt(100, 25, timeText, 1);
    
    // Date (simulated)
    display.printCentered(45, "2024-01-15", 1);
    
    // Blinking colon
    if ((frame / 10) % 2) {
        display.fillRect(62, 22, 2, 2, COLOR_WHITE);
        display.fillRect(62, 26, 2, 2, COLOR_WHITE);
    }
    
    display.show();
}

/**
 * Example 12: Audio Visualizer
 */
void exampleAudioVisualizer(SSD1306_Display& display, uint32_t frame) {
    display.clear();
    
    // Simulate audio levels
    uint8_t levels[16];
    for (int i = 0; i < 16; i++) {
        levels[i] = 20 + 30 * sin(frame * 0.05 + i * 0.3) + 
                    10 * sin(frame * 0.1 + i * 0.7);
    }
    
    // Draw frequency bars
    for (int i = 0; i < 16; i++) {
        int x = 8 + i * 7;
        int height = levels[i];
        int y = 50 - height;
        
        display.fillRect(x, y, 5, height, COLOR_WHITE);
    }
    
    // Title
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    display.printCentered(2, "Audio Visualizer", 1);
    
    // Peak indicator
    uint8_t peak = 0;
    for (int i = 0; i < 16; i++) {
        if (levels[i] > peak) peak = levels[i];
    }
    
    if (peak > 45) {
        display.fillCircle(120, 10, 3, COLOR_WHITE);
    }
    
    display.show();
}

/**
 * Example 13: Game of Life Cellular Automaton
 */
void exampleGameOfLife(SSD1306_Display& display, uint32_t frame) {
    static bool grid[16][8] = {false};
    static bool initialized = false;
    
    if (!initialized) {
        // Initialize with a glider pattern
        grid[2][1] = true;
        grid[3][2] = true;
        grid[4][2] = true;
        grid[2][3] = true;
        grid[3][3] = true;
        initialized = true;
    }
    
    display.clear();
    
    // Draw current state
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 8; y++) {
            if (grid[x][y]) {
                display.fillRect(x * 8, y * 8, 7, 7, COLOR_WHITE);
            }
        }
    }
    
    // Title
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    display.printCentered(2, "Game of Life", 1);
    
    // Update grid every 20 frames
    if (frame % 20 == 0) {
        bool newGrid[16][8] = {false};
        
        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 8; y++) {
                int neighbors = 0;
                
                // Count neighbors
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dx == 0 && dy == 0) continue;
                        
                        int nx = (x + dx + 16) % 16;
                        int ny = (y + dy + 8) % 8;
                        
                        if (grid[nx][ny]) neighbors++;
                    }
                }
                
                // Apply rules
                if (grid[x][y]) {
                    newGrid[x][y] = (neighbors == 2 || neighbors == 3);
                } else {
                    newGrid[x][y] = (neighbors == 3);
                }
            }
        }
        
        // Copy new state
        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 8; y++) {
                grid[x][y] = newGrid[x][y];
            }
        }
    }
    
    display.show();
}

/**
 * Example 14: Weather Display
 */
void exampleWeatherDisplay(SSD1306_Display& display, uint32_t frame) {
    display.clear();
    
    // Draw sun
    display.fillCircle(30, 25, 8, COLOR_WHITE);
    
    // Draw sun rays
    for (int i = 0; i < 8; i++) {
        float angle = i * 0.785; // 45 degrees
        int x1 = 30 + 12 * cos(angle);
        int y1 = 25 + 12 * sin(angle);
        int x2 = 30 + 16 * cos(angle);
        int y2 = 25 + 16 * sin(angle);
        display.drawLine(x1, y1, x2, y2, COLOR_WHITE);
    }
    
    // Draw clouds
    display.fillCircle(80, 20, 6, COLOR_WHITE);
    display.fillCircle(85, 20, 6, COLOR_WHITE);
    display.fillCircle(90, 20, 6, COLOR_WHITE);
    display.fillRect(80, 20, 10, 6, COLOR_WHITE);
    
    // Weather info
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    display.printAt(2, 40, "Temp: 72F", 1);
    display.printAt(2, 50, "Humidity: 45%", 1);
    
    // Animated raindrops
    if ((frame / 15) % 2) {
        display.fillRect(85, 30, 2, 4, COLOR_WHITE);
        display.fillRect(88, 32, 2, 4, COLOR_WHITE);
    }
    
    display.show();
}

/**
 * Example 15: Retro Game - Pong
 */
void examplePongGame(SSD1306_Display& display, uint32_t frame) {
    display.clear();
    
    // Paddle positions
    int leftPaddle = 20 + 10 * sin(frame * 0.05);
    int rightPaddle = 20 + 10 * sin(frame * 0.05 + 3.14);
    
    // Ball position
    int ballX = 64 + 40 * sin(frame * 0.1);
    int ballY = 32 + 20 * cos(frame * 0.15);
    
    // Draw paddles
    display.fillRect(5, leftPaddle, 4, 12, COLOR_WHITE);
    display.fillRect(123, rightPaddle, 4, 12, COLOR_WHITE);
    
    // Draw ball
    display.fillCircle(ballX, ballY, 2, COLOR_WHITE);
    
    // Draw center line
    for (int y = 0; y < 64; y += 8) {
        display.fillRect(63, y, 2, 4, COLOR_WHITE);
    }
    
    // Score
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    display.printAt(20, 2, "3", 1);
    display.printAt(100, 2, "2", 1);
    
    display.show();
} 