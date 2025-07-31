#pragma once

#define SDCARD_PIN          10

#define LED_PIN             8
#define LEDS_MATRIX_X       8
#define LEDS_MATRIX_Y       8
#define LEDS_STRIP_SHORT    8
#define LEDS_JEWEL          7 // 7 LEDs in the middle of the strip
#define LEDS_RING_24        24
#define LEDS_RING_16        16
#define LEDS_LARGE_MATRIX_X 16
#define LEDS_LARGE_MATRIX_Y 16

/* LEDs order
 * 8x8 matrix 
 * 1x8 strip
 * 1x8 strip
 * 16x16 matrix
 * 24 ring
 * 16 ring
 * jewel 
 */

#define LEDS_MATRIX_1       LEDS_MATRIX_X * LEDS_MATRIX_Y

// Tally up the number of LEDs
// 1x 8x8 matrix
// 2x 8 strip
// 1x 16x16 matrix
// 1x 24 ring
// 1x 16 ring
// 1x jewel
#define NUM_LEDS 32 * 32

#define BRIGHTNESS  125
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#define PUSHBUTTON_PIN 2
#define PUSHBUTTON_PIN_SECONDARY 3
#define PUSHBUTTON_HOLD_TIME_MS 1000

#ifndef POTENTIOMETER_PIN_BRIGHTNESS
#define POTENTIOMETER_PIN_BRIGHTNESS A4
#endif

#ifndef POTENTIOMETER_PIN_SPEED
#define POTENTIOMETER_PIN_SPEED A5
#endif

#ifndef POTENTIOMETER_PIN_EXTRA
#define POTENTIOMETER_PIN_EXTRA A6
#endif

#include <stdint.h>
#include <FastLED.h>

using void_ftn_ptr = void(*)(void);

using index_vector1 = fl::FixedVector<int, 1>;
using index_vector8 = fl::FixedVector<int, 8>;
using index_vector16 = fl::FixedVector<int, 16>;
using index_vector32 = fl::FixedVector<int, 32>;
using index_vector64 = fl::FixedVector<int, 64>;

extern void GoToPattern(int patternIndex);

using max_index_vector = index_vector64;

static int CoordsToIndex(int x, int y)
{
    return y * LEDS_MATRIX_X + x;
}
