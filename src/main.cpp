#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 1

#include <FastLED.h>
#include <stdint.h>
#include "Behaviors/Noise.hpp"
#include "Utils.hpp"
#include "Globals.h"
#include "LightPlayer2.h"
#include "player.hpp"

#include <Utils.hpp>
#include <SPI.h>
#include <SD.h>


Light LightArr[NUM_LEDS];// storage for player

CRGB leds[NUM_LEDS];
// CRGB ledNoise[NUM_LEDS];

Rgbw rgbw = Rgbw(
	kRGBWDefaultColorTemp,
	kRGBWExactColors,      // Mode
	W3                     // W-placement
);

typedef WS2812<LED_PIN, RGB> ControllerT;  // RGB mode must be RGB, no re-ordering allowed.
static RGBWEmulatedController<ControllerT, GRB> rgbwEmu(rgbw);  // ordering goes here.

#include <Adafruit_NeoPixel.h>

void wait_for_serial_connection()
{
	uint32_t timeout_end = millis() + 2000;
	Serial.begin(9600);
	while (!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
}

constexpr uint8_t COLOR_MAX = uint8_t(255);

// CRGB colors[] = {
// 	CRGB(COLOR_MAX, 0, 0),
// 	CRGB(0, COLOR_MAX, 0),
// 	CRGB(0, 0, COLOR_MAX),
// 	CRGB(COLOR_MAX, 0, COLOR_MAX)
// };


void setup()
{
	wait_for_serial_connection(); // Optional, but seems to help Teensy out a lot.
	// Used for RGB (NOT RGBW) LED strip
	// FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	// Used for RGBW (ring/string/matrix)
	FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
	FastLED.setBrightness(BRIGHTNESS);
	FastLED.show();
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);

	initializePatterns(LightArr);
	// if (!SD.begin(4))
	// {
	// 	Serial.println("Failed to initialize SD card");
	// 	delay(10);
	// }
}



unsigned long maxDelay = 505;
unsigned long minDelay = 50;
fract8 curr = 0;
unsigned long getNextDelay(unsigned long i)
{
	return InterpolateCubicFloat(minDelay, maxDelay, i / 64.f);
}

unsigned long lastUpdateMs = 0;
NoiseVis noise;
// int sharedCurrentIndexState = 0;
unsigned long last_ms = 0;
bool moveToNextPattern = false;

void loop()
{
	unsigned long ms = millis();
	FastLED.clear();
	UpdatePattern(leds, LightArr, NUM_LEDS);
	last_ms = ms;
	FastLED.show();
	unsigned long nextDelay = 15;// getNextDelay(myPulser.GetCurrentIndex());
	delay(nextDelay);
}
