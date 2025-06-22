#pragma once


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
#define LEDS_LARGE_MATRIX_1 LEDS_LARGE_MATRIX_X * LEDS_LARGE_MATRIX_Y

// First strip starts at the end of the matrix
#define LEDS_STRIP_1_START  LEDS_MATRIX_1
// Second strip starts at the end of the first strip
#define LEDS_STRIP_2_START  LEDS_STRIP_1_START + LEDS_STRIP_SHORT

// The 16x16 matrix starts at the end of the second strip
#define LEDS_LARGE_MATRIX_START  LEDS_STRIP_2_START + LEDS_STRIP_SHORT

// The 24 ring starts at the end of the 16x16 matrix
#define LEDS_RING_24_START  LEDS_LARGE_MATRIX_START + LEDS_LARGE_MATRIX_1

// The 16 ring starts at the end of the 24 ring
#define LEDS_RING_16_START  LEDS_RING_24_START + LEDS_RING_24

// Finally, the jewel starts at the end of the 16 ring
#define LEDS_JEWEL_START  LEDS_RING_16_START + LEDS_RING_16

// Tally up the number of LEDs
// 1x 8x8 matrix
// 2x 8 strip
// 1x 16x16 matrix
// 1x 24 ring
// 1x 16 ring
// 1x jewel
#define NUM_LEDS LEDS_MATRIX_1 + (2 * LEDS_STRIP_SHORT) + LEDS_LARGE_MATRIX_1 + LEDS_RING_24 + LEDS_RING_16 + LEDS_JEWEL

#define BRIGHTNESS  125
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#define PUSHBUTTON_PIN 2
#define PUSHBUTTON_PIN_SECONDARY 3
#define ONLY_PUSHBUTTON_PATTERN_CHANGE 1
#define PUSHBUTTON_HOLD_TIME_MS 1000

#define POTENTIOMETER_PIN_BRIGHTNESS A4
#define POTENTIOMETER_PIN_SPEED A5
#define POTENTIOMETER_PIN_EXTRA A6

#include <stdint.h>
#include <FastLED.h>

using void_ftn_ptr = void(*)(void);

using index_vector1 = fl::FixedVector<int, 1>;
using index_vector8 = fl::FixedVector<int, 8>;
using index_vector16 = fl::FixedVector<int, 16>;
using index_vector32 = fl::FixedVector<int, 32>;
using index_vector64 = fl::FixedVector<int, 64>;

using max_index_vector = index_vector64;

static int CoordsToIndex(int x, int y)
{
    return y * LEDS_MATRIX_X + x;
}
