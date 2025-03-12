#pragma once

#include <FastLED.h>
#include "../Globals.h"

int CoordsToIndex(int x, int y)
{
    return y * LEDS_MATRIX_X + x;
}

// ringNumber counting from 0
fl::FixedVector<int, 32> GetCoordsForRing(int ringNumber)
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
    int ringOffset = ringNumber + 1;
    fl::FixedVector<int, 32> indices;
    for (int x = ringNumber; x < LEDS_MATRIX_X - ringNumber; ++x)
    {
        const auto idx = CoordsToIndex(x, ringNumber);
        indices.push_back(idx);
    }
    for (int x = ringNumber; x < LEDS_MATRIX_X - ringNumber; ++x)
    {
        const auto idx = CoordsToIndex(x, LEDS_MATRIX_Y - ringNumber - 1);
        indices.push_back(idx);
    }

    // Left and right have their Y's with 1 higher (in value) at the top and 1 lower (in value) and the bottom
    for (int y = ringNumber + 1; y < LEDS_MATRIX_Y - ringNumber - 1; ++y)
    {
        const auto idx = CoordsToIndex(ringNumber, y);
        indices.push_back(idx);
    }
    for (int y = ringNumber + 1; y < LEDS_MATRIX_Y - ringNumber - 1; ++y)
    {
        const auto idx = CoordsToIndex(LEDS_MATRIX_X - ringNumber - 1, y);
        indices.push_back(idx);
    }
    return indices;

}

static const auto ring0 = GetCoordsForRing(0);
static const auto ring1 = GetCoordsForRing(1);
static const auto ring2 = GetCoordsForRing(2);
static const auto ring3 = GetCoordsForRing(3);

void DrawRing(CRGB *leds, const fl::FixedVector<int, 32> &indices, const CRGB &color)
{
    for (const auto i : indices)
    {
        leds[i] = color;
    }
}

void DrawRing(int index, CRGB *leds, const CRGB &color)
{
    switch (index)
    {
        case 0:
            DrawRing(leds, ring0, color);
            break;
        case 1:
            DrawRing(leds, ring1, color);
            break;
        case 2:
            DrawRing(leds, ring2, color);
            break;
        case 3:
            DrawRing(leds, ring3, color);
            break;
    }
}

// int currentRing = 0;
// int maxRing = 3;
// unsigned long wait_delay = 2000;
// unsigned long hold_ring_time = 500;
// unsigned long current_ring_hold_time = 0;

// bool waiting = false;
// bool holding = false;


