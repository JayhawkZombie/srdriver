#pragma once

#define LED_PIN     8
#define LEDS_MATRIX_X 8
#define LEDS_MATRIX_Y 8
#define NUM_LEDS    LEDS_MATRIX_X * LEDS_MATRIX_Y
#define BRIGHTNESS  55
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#include <stdint.h>

using void_ftn_ptr = void(*)(void);
