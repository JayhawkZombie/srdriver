#pragma once

#include "../hal/SSD_1306Component.h"

// Example function declarations
void exampleBasicText(SSD1306_Display& display);
void exampleProgressBar(SSD1306_Display& display, uint8_t percent);
void exampleBarGraph(SSD1306_Display& display);
void exampleShapes(SSD1306_Display& display);
void exampleBouncingBall(SSD1306_Display& display, uint32_t frame);
void exampleSystemStatus(SSD1306_Display& display, uint32_t uptime, uint8_t brightness, const char* pattern);
void exampleMenu(SSD1306_Display& display, uint8_t selectedItem);
void exampleLoading(SSD1306_Display& display, uint32_t frame); 