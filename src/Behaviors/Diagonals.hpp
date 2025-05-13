#pragma once
#include "../Globals.h"

index_vector8 GetIndicesForDiagonal(int quadrant)
{
    index_vector8 indices;
    int xdir = 0, ydir = 0;
    int startx, starty = 0;
    switch (quadrant)
    {
        case 0: {
            xdir = -1;
            ydir = -1;
            startx = (LEDS_MATRIX_X / 2) - 1;
            starty = (LEDS_MATRIX_Y / 2) - 1;
            break;
        }
        case 1: {
            xdir = 1;
            ydir = -1;
            startx = (LEDS_MATRIX_X / 2);
            starty = (LEDS_MATRIX_Y / 2) - 1;
            break;
        }
        case 2: {
            xdir = 1;
            ydir = 1;
            startx = (LEDS_MATRIX_X / 2);
            starty = (LEDS_MATRIX_Y / 2);
            break;
        }
        case 3: {
            xdir = -1;
            ydir = 1;
            startx = (LEDS_MATRIX_X / 2) - 1;
            starty = (LEDS_MATRIX_Y / 2);
            break;
        }
    }
    int currentx = startx, currenty = starty;
    while (currentx >= 0 && currenty >= 0 && currentx < LEDS_MATRIX_X && currenty < LEDS_MATRIX_Y) {
        indices.push_back(CoordsToIndex(currentx, currenty));
        currentx += xdir;
        currenty += ydir;
    }
    return indices;
}
