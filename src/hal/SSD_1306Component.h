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

class SSD1306_Display {
private:
    Adafruit_SSD1306 display;
    bool initialized;

public:
    SSD1306_Display() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), initialized(false) {}

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

    // Utility functions
    bool isInitialized() { return initialized; }

    // Quick setup function
    void setupDisplay()
    {
        if (begin())
        {
            Serial.println("SSD1306 initialized successfully!");
        }
        else
        {
            Serial.println("SSD1306 initialization failed!");
        }
    }
};
