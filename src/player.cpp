#include "player.hpp"
#include "LightPlayer2.h"
#include "Behaviors/Ring.hpp"
#include "Behaviors/ColumnsRows.hpp"
#include "Behaviors/Diagonals.hpp"
#include "Utils.hpp"

unsigned int patternIndex[] = { 1,2,4,5,3,6,0 };
unsigned int patternLength[] = { 64,64,64,64,64,10,30 };
unsigned int stepPause[] = { 1,1,2,1,1,5,1 };
unsigned int Param[] = { 3,5,1,4,1,1,1 };
fl::FixedVector<int, LEDS_MATRIX_Y> sharedIndices;

patternData pattData[16];
uint8_t stateData[24];
fl::FixedVector<char, 5> patternOrder;

LightPlayer2 LtPlay2;
Light onLt(255, 0, 0);
Light offLt(0, 0, 255);

void initializePatterns(Light* LightArr) {
    pattData[0].init(1, 1, 5);
    pattData[1].init(2, 1, 3);
    pattData[2].init(7, 8, 10);
    pattData[3].init(100, 20, 1);
    pattData[4].init(3, 1, 1);
    pattData[5].init(4, 1, 1);
    pattData[6].init(5, 1, 3);
    pattData[7].init(6, 8, 12);
    pattData[8].init(10, 2, 1);
    pattData[9].init(11, 2, 1);
    pattData[10].init(12, 2, 1);
    pattData[11].init(13, 2, 1);
    pattData[12].init(14, 4, 1);
    pattData[13].init(15, 4, 1);
    pattData[14].init(16, 2, 1);
    pattData[15].init(0, 30, 1);

    LtPlay2.init(LightArr[0], 8, 8, pattData[0], 15);

    stateData[0] = 60;
    stateData[1] = 36;
    stateData[2] = 36;
    stateData[3] = 36;
    stateData[4] = 36;
    stateData[5] = 36;
    stateData[6] = 36;
    stateData[7] = 60;
    stateData[8] = 0;
    stateData[9] = 0;
    stateData[10] = 60;
    stateData[11] = 60;
    stateData[12] = 60;
    stateData[13] = 60;
    stateData[14] = 0;
    stateData[15] = 0;
    stateData[16] = 0;
    stateData[17] = 0;
    stateData[18] = 255;
    stateData[19] = 129;
    stateData[20] = 129;
    stateData[21] = 255;
    stateData[22] = 0;
    stateData[23] = 0;

    LtPlay2.setStateData(stateData, 24);

    patternOrder.push_back('R');
    patternOrder.push_back('D');
    patternOrder.push_back('C');
    patternOrder.push_back('Z');
    patternOrder.push_back('X');
}

int sharedCurrentIndexState = 0;
int currentPatternIndex = 0;

void GoToNextPattern() {
    currentPatternIndex++;
    sharedCurrentIndexState = 0;
}

void DrawError(CRGB* leds, const CRGB &color)
{
    for (int i = 0; i < LEDS_MATRIX_X; i += 2)
    {
        leds[i] = color;
    }
}

void UpdatePattern(CRGB* leds, Light* LightArr, int numLeds) {
    const char currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
    switch (currentPattern) {
        case 'D':
            LtPlay2.update(onLt, offLt);
            for (int i = 0; i < numLeds; ++i) {
                leds[i].r = LightArr[i].r;
                leds[i].g = LightArr[i].g;
                leds[i].b = LightArr[i].b;
            }
            sharedCurrentIndexState++;
            if (sharedCurrentIndexState >= 100) {
                GoToNextPattern();
            }
            break;
        case 'R':
            DrawRing(sharedCurrentIndexState % 4, leds, CRGB::DarkRed);
            sharedCurrentIndexState++;
            if (sharedCurrentIndexState >= 16) {
                GoToNextPattern();
            }
            break;
        case 'C':
            sharedIndices = GetIndicesForColumn(sharedCurrentIndexState % 8);
            DrawColumnOrRow(leds, sharedIndices, CRGB::DarkBlue);
            sharedCurrentIndexState++;
            if (sharedCurrentIndexState >= 16) {
                GoToNextPattern();
            }
            break;
        case 'Z':
            sharedIndices = GetIndicesForRow(sharedCurrentIndexState % 8);
            DrawColumnOrRow(leds, sharedIndices, CRGB::DarkGreen);
            sharedCurrentIndexState++;
            if (sharedCurrentIndexState >= 16) {
                GoToNextPattern();
            }
            break;
        case 'X':
            sharedIndices = GetIndicesForDiagonal(sharedCurrentIndexState % 4);
            DrawColumnOrRow(leds, sharedIndices, CRGB::SlateGray);
            sharedCurrentIndexState++;
            if (sharedCurrentIndexState >= 16) {
                GoToNextPattern();
            }
            break;
        default:
            DrawError(leds, CRGB::Red);
            break;
    }
}
