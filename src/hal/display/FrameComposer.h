#pragma once

#include "DisplayBuffer.h"
#include <Arduino.h>

/**
 * FrameComposer - Composes complete display frames with overlays
 * 
 * Handles the composition of main content, system stats, and banner overlays
 * into a single DisplayBuffer that can be submitted to the display queue.
 */
class FrameComposer {
private:
    DisplayBuffer frameBuffer;
    Adafruit_SSD1306* gfx;
    
    // System stats for overlay
    uint32_t freeHeap;
    uint32_t totalHeap;
    uint8_t taskCount;
    uint32_t cpuFreq;
    uint32_t uptime;
    uint8_t heapUsagePercent;
    uint8_t powerScore;
    
public:
    FrameComposer();
    ~FrameComposer();
    
    // Update system statistics
    void updateSystemStats();
    
    // Compose a complete frame
    DisplayBuffer* composeFrame(const DisplayBuffer* mainContent = nullptr, const String& bannerText = "");
    
    // Individual rendering methods
    void renderMainContent(const DisplayBuffer* mainContent);
    void renderSystemStats();
    void renderBanner(const String& bannerText);
    
    // Get the composed frame
    DisplayBuffer* getFrame() { return &frameBuffer; }
}; 