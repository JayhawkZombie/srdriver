#pragma once

#define LED_PIN     8
#define LEDS_MATRIX_X 16
#define LEDS_MATRIX_Y 16
#define NUM_LEDS    LEDS_MATRIX_X * LEDS_MATRIX_Y
#define BRIGHTNESS  55
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#include <stdint.h>
#include <FastLED.h>

using void_ftn_ptr = void(*)(void);

using index_vector8 = fl::FixedVector<int, 8>;
using index_vector16 = fl::FixedVector<int, 16>;
using index_vector32 = fl::FixedVector<int, 32>;

static int CoordsToIndex(int x, int y)
{
    return y * LEDS_MATRIX_X + x;
}
