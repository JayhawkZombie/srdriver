#pragma once


#define LED_PIN             8
#define LEDS_MATRIX_X       8
#define LEDS_MATRIX_Y       8
#define LEDS_STRIP_SHORT    8
#define LEDS_JEWEL          7 // 7 LEDs in the middle of the strip


#define LEDS_MATRIX_1       LEDS_MATRIX_X * LEDS_MATRIX_Y
#define LEDS_STRIP_1_START  LEDS_MATRIX_1
#define LEDS_STRIP_2_START  LEDS_STRIP_1_START + LEDS_STRIP_SHORT
#define LEDS_JEWEL_START    LEDS_STRIP_2_START + LEDS_STRIP_SHORT

#define NUM_LEDS    LEDS_MATRIX_1 + ( 2 * LEDS_STRIP_SHORT) + LEDS_JEWEL

#define BRIGHTNESS  125
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#define PUSHBUTTON_PIN 2
#define ONLY_PUSHBUTTON_PATTERN_CHANGE 1
#define PUSHBUTTON_HOLD_TIME_MS 1000

#define POTENTIOMETER_PIN_BRIGHTNESS A4

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
