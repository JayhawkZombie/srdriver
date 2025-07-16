#pragma once

#include <stdint.h>
#include <Arduino.h>

// Display dimensions
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)

// Color zones for convenience methods
#define YELLOW_ZONE_HEIGHT 16  // Top 16 pixels (2 rows) are yellow
#define BLUE_ZONE_HEIGHT 48    // Bottom 48 pixels (6 rows) are blue
#define YELLOW_ZONE_Y 0        // Yellow zone starts at y=0
#define BLUE_ZONE_Y 16         // Blue zone starts at y=16

// Color constants
#define COLOR_BLACK 0
#define COLOR_WHITE 1

/**
 * DisplayBuffer - Device-independent drawing buffer for OLED displays
 * 
 * Provides a GPU-like rendering API that works on in-memory buffers.
 * Drawing operations are fast and don't require hardware access.
 * The buffer can then be uploaded to the physical display when ready.
 */
class DisplayBuffer {
private:
    uint8_t buffer[DISPLAY_BUFFER_SIZE];  // 128x64 monochrome = 1024 bytes
    bool dirty;                           // Content has changed since last upload
    bool ready;                           // Buffer is ready for display
    
    // Helper methods for pixel manipulation
    void setPixel(int x, int y, bool color);
    bool getPixel(int x, int y) const;
    bool isValidCoordinate(int x, int y) const;

public:
    DisplayBuffer();
    
    // Buffer management
    void clear();
    void markReady();
    void markDirty();
    bool isReady() const { return ready; }
    bool isDirty() const { return dirty; }
    const uint8_t* getBuffer() const { return buffer; }
    uint8_t* getBuffer() { return buffer; }
    
    // Basic drawing primitives
    void drawPixel(int x, int y, bool color = COLOR_WHITE);
    void drawLine(int x0, int y0, int x1, int y1, bool color = COLOR_WHITE);
    void drawRect(int x, int y, int w, int h, bool color = COLOR_WHITE);
    void fillRect(int x, int y, int w, int h, bool color = COLOR_WHITE);
    void drawCircle(int x, int y, int radius, bool color = COLOR_WHITE);
    void fillCircle(int x, int y, int radius, bool color = COLOR_WHITE);
    void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, bool color = COLOR_WHITE);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, bool color = COLOR_WHITE);
    
    // Text rendering
    void drawText(int x, int y, const char* text, int size = 1, bool color = COLOR_WHITE);
    void drawTextCentered(int y, const char* text, int size = 1, bool color = COLOR_WHITE);
    
    // Advanced drawing functions
    void drawProgressBar(int x, int y, int w, int h, uint8_t percent, bool color = COLOR_WHITE);
    void drawBarGraph(int x, int y, int w, int h, uint8_t values[], uint8_t numValues, bool color = COLOR_WHITE);
    
    // Convenience methods for main area (blue region)
    void drawMainAreaText(int x, int y, const char* text, int size = 1, bool color = COLOR_WHITE);
    void drawMainAreaTextCentered(int y, const char* text, int size = 1, bool color = COLOR_WHITE);
    void drawMainAreaRect(int x, int y, int w, int h, bool color = COLOR_WHITE);
    void fillMainAreaRect(int x, int y, int w, int h, bool color = COLOR_WHITE);
    void drawMainAreaLine(int x0, int y0, int x1, int y1, bool color = COLOR_WHITE);
    void drawMainAreaProgressBar(int x, int y, int w, int h, uint8_t percent, bool color = COLOR_WHITE);
    
    // Utility functions
    int getTextWidth(const char* text, int size = 1) const;
    int getTextHeight(int size = 1) const;
    void copyFrom(const DisplayBuffer& other);
    void copyRegion(const DisplayBuffer& other, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY);
}; 