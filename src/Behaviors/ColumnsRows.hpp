#pragma once

#include <FastLED.h>
#include "../Globals.h"

void DrawColumnOrRow(CRGB *leds, const fl::FixedVector<int, LEDS_MATRIX_Y> &vec, const CRGB &color)
{
    for (const auto &idx : vec) {
        leds[idx] = color;
    }
}

fl::FixedVector<int, LEDS_MATRIX_Y> GetIndicesForColumn(int column)
{
    fl::FixedVector<int, LEDS_MATRIX_Y> indices;
    for (int y = 0; y < LEDS_MATRIX_Y; ++y) {
        indices.push_back(CoordsToIndex(column, y));
    }
    return indices;
}

fl::FixedVector<int, LEDS_MATRIX_Y> GetIndicesForRow(int row)
{
    fl::FixedVector<int, LEDS_MATRIX_Y> indices;
    for (int y = 0; y < LEDS_MATRIX_Y; ++y)
    {
        indices.push_back(CoordsToIndex(y, row));
    }
    return indices;
}
