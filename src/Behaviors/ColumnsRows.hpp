#pragma once

#include <FastLED.h>
#include "../Globals.h"

inline void DrawColumnOrRow(CRGB *leds, const fl::FixedVector<int, LEDS_MATRIX_Y> &vec, const CRGB &color)
{
    for (const auto &idx : vec) {
        leds[idx] = color;
    }
}

inline fl::FixedVector<int, LEDS_MATRIX_Y> GetIndicesForColumn(int column)
{
    fl::FixedVector<int, LEDS_MATRIX_Y> indices;
    for (int y = 0; y < LEDS_MATRIX_Y; ++y) {
        indices.push_back(CoordsToIndex(column, y));
    }
    return indices;
}

inline fl::FixedVector<int, LEDS_MATRIX_X> GetIndicesForRow(int row)
{
    fl::FixedVector<int, LEDS_MATRIX_X> indices;
    for (int x = 0; x < LEDS_MATRIX_X; ++x)
    {
        indices.push_back(CoordsToIndex(x, row));
    }
    return indices;
}
