#pragma once

#define LED_PIN     8
#define NUM_LEDS    8 * 8
#define BRIGHTNESS  55
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#include <stdint.h>

using void_ftn_ptr = void(*)(void);
