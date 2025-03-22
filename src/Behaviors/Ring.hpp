#pragma once

#include <FastLED.h>
#include "../Globals.h"

// ringNumber counting from 0
inline fl::FixedVector<int, 32> GetCoordsForRing(int ringNumber)
{
    // Number of steps in from the edge we go
    // Bounds will be, where i = ringNumber (from 0)
    /**
     * Top and bottom rows have their corners
     * Logically top and bottom of right and left half are ignored
     * If
     * 		TLx = i            inset by row index
     * 		TLy = i            inset by row inset
     *      TRx = Dx - 1 - i   Top right is Dims_x - 1 - rowIndex
     *      TRy = Ty - 1 - i
     * (i, i) -> (Dx - i - 1, i): top
     * (Dx - i - 1, i + 1) -> (Dx - i - 1, Dy - i - 1 + 1): right, +1 to y to avoid extra corners
     * (Dx - 1 - 1, Dy - i - 1) -> ()
     *
     */
    // int ringOffset = ringNumber + 1;
    fl::FixedVector<int, 32> indices;
    for (int x = ringNumber; x < LEDS_MATRIX_X - ringNumber; ++x)
    {
        const auto idx = CoordsToIndex(x, ringNumber);
        indices.push_back(idx);
    }
    // Left and right have their Y's with 1 higher (in value) at the top and 1 lower (in value) and the bottom
    for (int y = ringNumber + 1; y < LEDS_MATRIX_Y - ringNumber - 1; ++y)
    {
        const auto idx = CoordsToIndex(ringNumber, y);
        indices.push_back(idx);
    }

    // for (int x = ringNumber; x < LEDS_MATRIX_X - ringNumber; ++x)
    // {
    //     const auto idx = CoordsToIndex(x, LEDS_MATRIX_Y - ringNumber - 1);
    //     indices.push_back(idx);
    // }

    for (int x = LEDS_MATRIX_X - ringNumber - 1; x >= ringNumber; --x)
    {
        const auto idx = CoordsToIndex(x, LEDS_MATRIX_Y - ringNumber - 1);
        indices.push_back(idx);
    }


    // Left and right have their Y's with 1 higher (in value) at the top and 1 lower (in value) and the bottom
    // for (int y = ringNumber + 1; y < LEDS_MATRIX_Y - ringNumber - 1; ++y)
    // {
    //     const auto idx = CoordsToIndex(LEDS_MATRIX_X - ringNumber - 1, y);
    //     indices.push_back(idx);
    // }
    for (int y = LEDS_MATRIX_Y - ringNumber - 2; y >= ringNumber + 1; --y)
    {
        const auto idx = CoordsToIndex(LEDS_MATRIX_X - ringNumber - 1, y);
        indices.push_back(idx);
    }

    return indices;

}

inline void DrawRing(CRGB *leds, const fl::FixedVector<int, 32> &indices, const CRGB &color)
{
    for (const auto i : indices)
    {
        leds[i] = color;
    }
}

inline void DrawRing(int index, CRGB *leds, const CRGB &color)
{
    const auto indices = GetCoordsForRing(index);
    DrawRing(leds, indices, color);
}


