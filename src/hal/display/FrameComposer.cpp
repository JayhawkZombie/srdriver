#include "FrameComposer.h"
#include <ESP.h>

FrameComposer::FrameComposer() : gfx(nullptr) {
    updateSystemStats();
}

FrameComposer::~FrameComposer() {
    if (gfx) {
        frameBuffer.releaseGFXRenderer(gfx);
    }
}

void FrameComposer::updateSystemStats() {
    freeHeap = ESP.getFreeHeap();
    totalHeap = ESP.getHeapSize();
    taskCount = uxTaskGetNumberOfTasks();
    cpuFreq = ESP.getCpuFreqMHz();
    uptime = millis() / 1000;
    
    // Update memory usage percentage
    heapUsagePercent = (totalHeap > 0) ? (100 - (freeHeap * 100 / totalHeap)) : 0;
    
    // Calculate power efficiency score (simplified)
    powerScore = 100;
    if (cpuFreq > 240) powerScore -= 20;
    else if (cpuFreq > 160) powerScore -= 10;
    if (taskCount > 10) powerScore -= 20;
    else if (taskCount > 6) powerScore -= 10;
}

DisplayBuffer* FrameComposer::composeFrame(const DisplayBuffer* mainContent, const String& bannerText) {
    // Get GFX renderer
    gfx = frameBuffer.getGFXRenderer();
    
    // Clear the frame buffer
    gfx->clearDisplay();
    
    // 1. Render main content (if provided)
    renderMainContent(mainContent);
    
    // 2. Render system stats overlay (always on top)
    renderSystemStats();
    
    // 3. Render banner (if provided, on very top)
    if (bannerText.length() > 0) {
        renderBanner(bannerText);
    }
    
    // Release GFX renderer and mark frame as ready
    frameBuffer.releaseGFXRenderer(gfx);
    gfx = nullptr;
    frameBuffer.markReady();
    
    return &frameBuffer;
}

void FrameComposer::renderMainContent(const DisplayBuffer* mainContent) {
    if (mainContent && mainContent->isReady()) {
        // Copy main content to the frame buffer
        frameBuffer.copyFrom(*mainContent);
    } else {
        // No main content - render default "No Content" message
        gfx->setTextSize(1);
        gfx->setCursor(2, 20);
        gfx->print("No Display Content");
    }
}

void FrameComposer::renderSystemStats() {
    // Render system stats in the blue region (main area)
    gfx->setTextSize(1);
    gfx->setTextColor(SSD1306_WHITE);
    
    // Draw system status header
    gfx->setCursor(2, BLUE_ZONE_Y + 2);
    gfx->print("System Status");
    
    // Draw separator line
    gfx->drawLine(0, BLUE_ZONE_Y + 12, 128, BLUE_ZONE_Y + 12, SSD1306_WHITE);
    
    // Draw uptime
    char uptimeText[32];
    snprintf(uptimeText, sizeof(uptimeText), "Uptime: %ds", uptime);
    gfx->setCursor(2, BLUE_ZONE_Y + 20);
    gfx->print(uptimeText);
    
    // Draw memory usage with progress bar
    gfx->setCursor(2, BLUE_ZONE_Y + 32);
    gfx->print("Memory:");
    
    // Draw progress bar
    int barX = 50;
    int barY = BLUE_ZONE_Y + 32;
    int barW = 60;
    int barH = 8;
    
    // Draw border
    gfx->drawRect(barX, barY, barW, barH, SSD1306_WHITE);
    
    // Draw fill
    int fillWidth = (barW - 2) * heapUsagePercent / 100;
    if (fillWidth > 0) {
        gfx->fillRect(barX + 1, barY + 1, fillWidth, barH - 2, SSD1306_WHITE);
    }
    
    // Draw task count
    char taskText[32];
    snprintf(taskText, sizeof(taskText), "Tasks: %d", taskCount);
    gfx->setCursor(2, BLUE_ZONE_Y + 44);
    gfx->print(taskText);
    
    // Draw CPU frequency
    char cpuText[32];
    snprintf(cpuText, sizeof(cpuText), "CPU: %dMHz", cpuFreq);
    gfx->setCursor(2, BLUE_ZONE_Y + 56);
    gfx->print(cpuText);
    
    // Draw power efficiency score
    char powerText[32];
    snprintf(powerText, sizeof(powerText), "Power: %d%%", powerScore);
    gfx->setCursor(70, BLUE_ZONE_Y + 56);
    gfx->print(powerText);
}

void FrameComposer::renderBanner(const String& bannerText) {
    // Render banner in the yellow region (top area)
    gfx->setTextSize(1);
    gfx->setTextColor(SSD1306_WHITE);
    
    // Center the banner text
    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds(bannerText.c_str(), 0, 0, &x1, &y1, &w, &h);
    int16_t x = (DISPLAY_WIDTH - w) / 2;
    
    gfx->setCursor(x, 2);
    gfx->print(bannerText);
} 