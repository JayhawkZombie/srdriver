#include "../hal/PinMappings.hpp"
#include "SetupLEDs.h"

bool InitLEDsOnPin(pin_t pin, int numLEDs, CRGB* leds, int startLED, int endLED)
{
    switch (pin)
    {
        case D0:
            FastLED.addLeds<WS2812B, D0, GRB>(leds, startLED, endLED);
            break;
        case D1:
            FastLED.addLeds<WS2812B, D1, GRB>(leds, startLED, endLED);
            break;
        case D2:
            FastLED.addLeds<WS2812B, D2, GRB>(leds, startLED, endLED);
            break;
        case D3:
            FastLED.addLeds<WS2812B, D3, GRB>(leds, startLED, endLED);
            break;
        case D4:
            FastLED.addLeds<WS2812B, D4, GRB>(leds, startLED, endLED);
            break;
        case D5:
            FastLED.addLeds<WS2812B, D5, GRB>(leds, startLED, endLED);
            break;
        case D6:
            FastLED.addLeds<WS2812B, D6, GRB>(leds, startLED, endLED);
            break;
        case D7:
            FastLED.addLeds<WS2812B, D7, GRB>(leds, startLED, endLED);
            break;
        case D8:
            FastLED.addLeds<WS2812B, D8, GRB>(leds, startLED, endLED);
            break;
        case D9:
            FastLED.addLeds<WS2812B, D9, GRB>(leds, startLED, endLED);
            break;
        case D10:
            FastLED.addLeds<WS2812B, D10, GRB>(leds, startLED, endLED);
            break;
        case D11:
            FastLED.addLeds<WS2812B, D11, GRB>(leds, startLED, endLED);
            break;
        case D12:
            FastLED.addLeds<WS2812B, D12, GRB>(leds, startLED, endLED);
            break;
        case D13:
            FastLED.addLeds<WS2812B, D13, GRB>(leds, startLED, endLED);
            break;
        default:
            return false;
    }
    return true;
}
