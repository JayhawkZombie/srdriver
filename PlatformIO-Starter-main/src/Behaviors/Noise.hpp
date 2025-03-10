#pragma once

#include <FastLED.h>
#include "../Globals.h"

// Time scaling factors for each component
#define TIME_FACTOR_HUE 60
#define TIME_FACTOR_SAT 100
#define TIME_FACTOR_VAL 100

class NoiseVis
{
public:
    void Update(uint32_t ms, CRGB *leds)
    {
        for (int i = 0; i < NUM_LEDS; i++)
        {
        	// Use different noise functions for each LED and each color component
        	uint8_t hue = inoise16(ms * TIME_FACTOR_HUE, i * 100, 0) >> 8;
        	uint8_t sat = inoise16(ms * TIME_FACTOR_SAT, i * 200, 1000) >> 8;
        	uint8_t val = inoise16(ms * TIME_FACTOR_VAL, i * 300, 2000) >> 8;

        	// Map the noise to full range for saturation and value
        	sat = map(sat, 0, 255, 30, 255);
        	val = map(val, 0, 255, 100, 255);

        	leds[i] = CHSV(hue, sat, val);
        }
    }
};
