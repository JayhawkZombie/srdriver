#include "LEDStorage.h"

#if SUPPORTS_LEDS
#include "Globals.h"

// FastLED array - this is what gets sent to hardware
CRGB leds[NUM_LEDS];

#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
// RGBW support
Rgbw rgbw = Rgbw(
    kRGBWDefaultColorTemp,
    kRGBWExactColors,      // Mode
    W3                     // W-placement
);

typedef WS2812<LED_PIN, RGB> ControllerT;  // RGB mode must be RGB, no re-ordering allowed
static RGBWEmulatedController<ControllerT, GRB> rgbwEmu(rgbw);  // ordering goes here
#endif

bool initializeFastLED() {
    // Black out LEDs first
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
    }
    
#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
    FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
#else
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
           .setCorrection(TypicalLEDStrip);
#endif
    
    FastLED.clear();
    FastLED.show();
    return true;
}

#endif // SUPPORTS_LEDS

