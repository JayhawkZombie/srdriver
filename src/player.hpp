#pragma once
#include "Globals.h"
#include <FastLED.h>
#include <Light.h>
#include <LightPlayer2.h>
extern unsigned int patternIndex[];
extern unsigned int patternLength[];
extern unsigned int stepPause[];
extern unsigned int Param[];

extern patternData pattData[16];
extern uint8_t stateData[24];
extern fl::FixedVector<char, 5> patternOrder;

void initializePatterns(Light* LightArr);
void GoToNextPattern();
void UpdatePattern(CRGB* leds, Light* LightArr, int numLeds);
