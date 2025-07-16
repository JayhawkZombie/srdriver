#include "DisplayBuffer.h"
#include <cstring>
#include <algorithm>

DisplayBuffer::DisplayBuffer() : dirty(false), ready(false), gfxRenderer(nullptr) {
    clear();
}

DisplayBuffer::~DisplayBuffer() {
    // Clean up the GFX renderer
    if (gfxRenderer) {
        delete gfxRenderer;
        gfxRenderer = nullptr;
    }
}

void DisplayBuffer::clear() {
    memset(buffer, 0, DISPLAY_BUFFER_SIZE);
    dirty = true;
    ready = false;
}

void DisplayBuffer::markReady() {
    ready = true;
    dirty = true;
}

void DisplayBuffer::markDirty() {
    dirty = true;
}

bool DisplayBuffer::isValidCoordinate(int x, int y) const {
    return x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT;
}

void DisplayBuffer::setPixel(int x, int y, bool color) {
    if (!isValidCoordinate(x, y)) return;
    
    // Calculate byte index and bit position
    int byteIndex = (y / 8) * DISPLAY_WIDTH + x;
    int bitPosition = y % 8;
    
    if (color) {
        buffer[byteIndex] |= (1 << bitPosition);
    } else {
        buffer[byteIndex] &= ~(1 << bitPosition);
    }
}

bool DisplayBuffer::getPixel(int x, int y) const {
    if (!isValidCoordinate(x, y)) return false;
    
    int byteIndex = (y / 8) * DISPLAY_WIDTH + x;
    int bitPosition = y % 8;
    
    return (buffer[byteIndex] & (1 << bitPosition)) != 0;
}

void DisplayBuffer::drawPixel(int x, int y, bool color) {
    setPixel(x, y, color);
    dirty = true;
}

void DisplayBuffer::drawLine(int x0, int y0, int x1, int y1, bool color) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    int x = x0, y = y0;
    
    while (true) {
        drawPixel(x, y, color);
        
        if (x == x1 && y == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void DisplayBuffer::drawRect(int x, int y, int w, int h, bool color) {
    drawLine(x, y, x + w - 1, y, color);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
    drawLine(x + w - 1, y + h - 1, x, y + h - 1, color);
    drawLine(x, y + h - 1, x, y, color);
}

void DisplayBuffer::fillRect(int x, int y, int w, int h, bool color) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            drawPixel(px, py, color);
        }
    }
}

void DisplayBuffer::drawCircle(int x, int y, int radius, bool color) {
    // Bresenham's circle algorithm
    int x0 = radius;
    int y0 = 0;
    int err = 0;
    
    while (x0 >= y0) {
        drawPixel(x + x0, y + y0, color);
        drawPixel(x + y0, y + x0, color);
        drawPixel(x - y0, y + x0, color);
        drawPixel(x - x0, y + y0, color);
        drawPixel(x - x0, y - y0, color);
        drawPixel(x - y0, y - x0, color);
        drawPixel(x + y0, y - x0, color);
        drawPixel(x + x0, y - y0, color);
        
        if (err <= 0) {
            y0 += 1;
            err += 2 * y0 + 1;
        }
        if (err > 0) {
            x0 -= 1;
            err -= 2 * x0 + 1;
        }
    }
}

void DisplayBuffer::fillCircle(int x, int y, int radius, bool color) {
    // Fill circle using horizontal lines
    for (int py = -radius; py <= radius; py++) {
        for (int px = -radius; px <= radius; px++) {
            if (px * px + py * py <= radius * radius) {
                drawPixel(x + px, y + py, color);
            }
        }
    }
}

void DisplayBuffer::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, bool color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void DisplayBuffer::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, bool color) {
    // Simple triangle fill using horizontal scan lines
    // This is a basic implementation - could be optimized
    
    // Sort vertices by y coordinate
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    
    // Fill triangle using horizontal lines
    for (int y = y0; y <= y2; y++) {
        int xl, xr;
        
        if (y <= y1) {
            // Upper half
            xl = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            xr = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        } else {
            // Lower half
            xl = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
            xr = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        }
        
        if (xl > xr) std::swap(xl, xr);
        
        for (int x = xl; x <= xr; x++) {
            drawPixel(x, y, color);
        }
    }
}

void DisplayBuffer::drawText(int x, int y, const char* text, int size, bool color) {
    // Simple 5x7 font implementation
    // This is a basic implementation - could be enhanced with proper font support
    
    int charWidth = 6 * size;
    int charHeight = 8 * size;
    
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        int charX = x + i * charWidth;
        
        // Simple character rendering (basic ASCII)
        if (c >= 32 && c <= 126) {
            // For now, draw a simple character representation
            // In a full implementation, you'd have a proper font bitmap
            if (c >= 'A' && c <= 'Z') {
                // Draw a simple letter representation
                fillRect(charX + 1, y, 4, charHeight, color);
                fillRect(charX, y + 1, 6, 2, color);
                fillRect(charX, y + charHeight - 3, 6, 2, color);
            } else if (c >= 'a' && c <= 'z') {
                // Draw lowercase letters
                fillRect(charX + 1, y + 2, 4, charHeight - 2, color);
                fillRect(charX, y + 1, 6, 2, color);
            } else if (c >= '0' && c <= '9') {
                // Draw numbers
                fillRect(charX + 1, y, 4, charHeight, color);
                fillRect(charX, y + 1, 6, 2, color);
                fillRect(charX, y + charHeight - 3, 6, 2, color);
            } else {
                // For other characters, draw a simple rectangle
                fillRect(charX, y, charWidth - 1, charHeight, color);
            }
        }
    }
}

void DisplayBuffer::drawTextCentered(int y, const char* text, int size, bool color) {
    int textWidth = getTextWidth(text, size);
    int x = (DISPLAY_WIDTH - textWidth) / 2;
    drawText(x, y, text, size, color);
}

void DisplayBuffer::drawProgressBar(int x, int y, int w, int h, uint8_t percent, bool color) {
    // Draw border
    drawRect(x, y, w, h, color);
    
    // Calculate fill width
    int fillWidth = (w - 2) * percent / 100;
    if (fillWidth > 0) {
        fillRect(x + 1, y + 1, fillWidth, h - 2, color);
    }
}

void DisplayBuffer::drawBarGraph(int x, int y, int w, int h, uint8_t values[], uint8_t numValues, bool color) {
    if (numValues == 0) return;
    
    int barWidth = w / numValues;
    int maxBarHeight = h - 2;
    
    for (uint8_t i = 0; i < numValues; i++) {
        int barHeight = (values[i] * maxBarHeight) / 255;
        int barX = x + (i * barWidth);
        int barY = y + h - barHeight - 1;
        fillRect(barX, barY, barWidth - 1, barHeight, color);
    }
}

// Convenience methods for main area (blue region)
void DisplayBuffer::drawMainAreaText(int x, int y, const char* text, int size, bool color) {
    drawText(x, y + BLUE_ZONE_Y, text, size, color);
}

void DisplayBuffer::drawMainAreaTextCentered(int y, const char* text, int size, bool color) {
    drawTextCentered(y + BLUE_ZONE_Y, text, size, color);
}

void DisplayBuffer::drawMainAreaRect(int x, int y, int w, int h, bool color) {
    drawRect(x, y + BLUE_ZONE_Y, w, h, color);
}

void DisplayBuffer::fillMainAreaRect(int x, int y, int w, int h, bool color) {
    fillRect(x, y + BLUE_ZONE_Y, w, h, color);
}

void DisplayBuffer::drawMainAreaLine(int x0, int y0, int x1, int y1, bool color) {
    drawLine(x0, y0 + BLUE_ZONE_Y, x1, y1 + BLUE_ZONE_Y, color);
}

void DisplayBuffer::drawMainAreaProgressBar(int x, int y, int w, int h, uint8_t percent, bool color) {
    drawProgressBar(x, y + BLUE_ZONE_Y, w, h, percent, color);
}

// Utility functions
int DisplayBuffer::getTextWidth(const char* text, int size) const {
    int charWidth = 6 * size;
    int width = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        width += charWidth;
    }
    return width;
}

int DisplayBuffer::getTextHeight(int size) const {
    return 8 * size;
}

void DisplayBuffer::copyFrom(const DisplayBuffer& other) {
    memcpy(buffer, other.buffer, DISPLAY_BUFFER_SIZE);
    dirty = true;
}

void DisplayBuffer::copyRegion(const DisplayBuffer& other, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY) {
    for (int y = 0; y < srcH; y++) {
        for (int x = 0; x < srcW; x++) {
            bool pixel = other.getPixel(srcX + x, srcY + y);
            setPixel(dstX + x, dstY + y, pixel);
        }
    }
    dirty = true;
}

// NEW: Adafruit GFX rendering support
Adafruit_SSD1306* DisplayBuffer::getGFXRenderer() {
    if (!gfxRenderer) {
        // Create a temporary SSD1306 object that points to our buffer
        gfxRenderer = new Adafruit_SSD1306(DISPLAY_WIDTH, DISPLAY_HEIGHT, nullptr, -1);
        
        // Initialize the renderer (this sets up the buffer format)
        gfxRenderer->begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false);
        
        // Set up basic state
        gfxRenderer->setTextColor(SSD1306_WHITE);
    }
    
    // Copy our buffer to the renderer's buffer before use
    uint8_t* rendererBuffer = gfxRenderer->getBuffer();
    if (rendererBuffer) {
        memcpy(rendererBuffer, buffer, DISPLAY_BUFFER_SIZE);
    }
    
    return gfxRenderer;
}

void DisplayBuffer::releaseGFXRenderer(Adafruit_SSD1306* renderer) {
    if (renderer && renderer == gfxRenderer) {
        // Copy the renderer's buffer back to our buffer
        uint8_t* rendererBuffer = renderer->getBuffer();
        if (rendererBuffer) {
            memcpy(buffer, rendererBuffer, DISPLAY_BUFFER_SIZE);
        }
        
        // Mark buffer as dirty since GFX operations modified it
        dirty = true;
        
        // Don't delete or null the renderer - we'll reuse it
        // The renderer stays allocated for the lifetime of this DisplayBuffer
    }
} 