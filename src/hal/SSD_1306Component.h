#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address (0x3C or 0x3D)

// I2C pins for Arduino Nano ESP32
#define SDA_PIN A4
#define SCL_PIN A5

// Color zones for yellow/blue display
#define YELLOW_ZONE_HEIGHT 16  // Top 16 pixels (2 rows) are yellow
#define BLUE_ZONE_HEIGHT 48    // Bottom 48 pixels (6 rows) are blue
#define YELLOW_ZONE_Y 0        // Yellow zone starts at y=0
#define BLUE_ZONE_Y 16         // Blue zone starts at y=16

// Color constants
#define COLOR_BLACK 0
#define COLOR_WHITE 1

class SSD1306_Display {
private:
    Adafruit_SSD1306 display;
    bool initialized;
    bool hasColorFilter;  // true for yellow/blue, false for B&W

public:
    SSD1306_Display(bool colorFilter = false)
        : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
        initialized(false),
        hasColorFilter(colorFilter)
    {}

    // Initialize the display
    bool begin()
    {
        Wire.begin(SDA_PIN, SCL_PIN);

        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
        {
            Serial.println(F("SSD1306 allocation failed"));
            return false;
        }

        initialized = true;
        display.clearDisplay();
        return true;
    }

    // Basic display functions
    void clear()
    {
        if (initialized) display.clearDisplay();
    }

    void show()
    {
        if (initialized) display.display();
    }

    // Text functions
    void setTextSize(uint8_t size)
    {
        if (initialized) display.setTextSize(size);
    }

    void setTextColor(uint16_t color)
    {
        if (initialized) display.setTextColor(color);
    }

    void setCursor(int16_t x, int16_t y)
    {
        if (initialized) display.setCursor(x, y);
    }

    void print(const char *text)
    {
        if (initialized) display.print(text);
    }

    void println(const char *text)
    {
        if (initialized) display.println(text);
    }

    // Enhanced text functions
    void printAt(int16_t x, int16_t y, const char *text, uint8_t size = 1)
    {
        if (!initialized) return;
        display.setTextSize(size);
        display.setCursor(x, y);
        display.print(text);
    }

    void printCentered(int16_t y, const char *text, uint8_t size = 1)
    {
        if (!initialized) return;
        display.setTextSize(size);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        int16_t x = (SCREEN_WIDTH - w) / 2;
        display.setCursor(x, y);
        display.print(text);
    }

    // Drawing functions
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    {
        if (initialized) display.drawRect(x, y, w, h, color);
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
    {
        if (initialized) display.drawLine(x0, y0, x1, y1, color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    {
        if (initialized) display.fillRect(x, y, w, h, color);
    }

    // Enhanced drawing functions
    void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color)
    {
        if (initialized) display.drawCircle(x, y, r, color);
    }

    void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color)
    {
        if (initialized) display.fillCircle(x, y, r, color);
    }

    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
    {
        if (initialized) display.drawTriangle(x0, y0, x1, y1, x2, y2, color);
    }

    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
    {
        if (initialized) display.fillTriangle(x0, y0, x1, y1, x2, y2, color);
    }

    // Progress bar functions
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, uint16_t color = COLOR_WHITE)
    {
        if (!initialized) return;

        // Draw border
        display.drawRect(x, y, w, h, color);

        // Calculate fill width
        int16_t fillWidth = (w - 2) * percent / 100;
        if (fillWidth > 0)
        {
            display.fillRect(x + 1, y + 1, fillWidth, h - 2, color);
        }
    }

    // Graph functions
    void drawBarGraph(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t values[], uint8_t numValues, uint16_t color = COLOR_WHITE)
    {
        if (!initialized || numValues == 0) return;

        int16_t barWidth = w / numValues;
        int16_t maxBarHeight = h - 2;

        for (uint8_t i = 0; i < numValues; i++)
        {
            int16_t barHeight = (values[i] * maxBarHeight) / 255;
            int16_t barX = x + (i * barWidth);
            int16_t barY = y + h - barHeight - 1;
            display.fillRect(barX, barY, barWidth - 1, barHeight, color);
        }
    }

    // Animation helper - scroll text
    void scrollText(int16_t y, const char *text, uint8_t size = 1, uint16_t delayMs = 100)
    {
        if (!initialized) return;

        display.setTextSize(size);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

        // Scroll from right to left
        for (int16_t x = SCREEN_WIDTH; x >= -w; x--)
        {
            display.clearDisplay();
            display.setCursor(x, y);
            display.print(text);
            display.display();
            delay(delayMs);
        }
    }

    // Color zone awareness functions
    bool isInYellowZone(int16_t y) const
    {
        return hasColorFilter && y < YELLOW_ZONE_HEIGHT;
    }

    bool isInBlueZone(int16_t y) const
    {
        return hasColorFilter && y >= YELLOW_ZONE_Y && y < (YELLOW_ZONE_Y + BLUE_ZONE_HEIGHT);
    }

    // Draw with color zone awareness
    void drawRectColorAware(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    {
        if (!initialized) return;

        // For color displays, we can't change the actual color, but we can log it
        if (hasColorFilter)
        {
            if (y < YELLOW_ZONE_HEIGHT)
            {
                // This will appear yellow
                display.drawRect(x, y, w, h, color);
            }
            else if (y >= YELLOW_ZONE_Y && y < (YELLOW_ZONE_Y + BLUE_ZONE_HEIGHT))
            {
                // This will appear blue
                display.drawRect(x, y, w, h, color);
            }
        }
        else
        {
            // Black and white display
            display.drawRect(x, y, w, h, color);
        }
    }

    // Utility functions
    bool isInitialized() { return initialized; }
    bool hasColorFilterDisplay() { return hasColorFilter; }

    // Quick setup function
    void setupDisplay()
    {
        if (begin())
        {
            Serial.println("SSD1306 initialized successfully!");
            if (hasColorFilter)
            {
                Serial.println("Color filter detected: Yellow/Blue zones");
            }
            else
            {
                Serial.println("Black and white display");
            }
        }
        else
        {
            Serial.println("SSD1306 initialization failed!");
        }
    }

    // Get display dimensions
    int16_t getWidth() const { return SCREEN_WIDTH; }
    int16_t getHeight() const { return SCREEN_HEIGHT; }
};
