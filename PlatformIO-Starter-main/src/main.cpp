#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 1

#include <FastLED.h>
#include <stdint.h>
#include "Globals.h"
#include "Behaviors/Pulser.hpp"
#include "Behaviors/ReversePulser.hpp"


CRGB leds[NUM_LEDS];

// Time scaling factors for each component
#define TIME_FACTOR_HUE 60
#define TIME_FACTOR_SAT 100
#define TIME_FACTOR_VAL 100

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
	Serial.begin(115200);
	while (!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
}

constexpr uint8_t COLOR_MAX = uint8_t(255);

CRGB colors[] = {
	CRGB(COLOR_MAX, 0,          0), // red
	CRGB(COLOR_MAX, COLOR_MAX,  0),
	CRGB(0, COLOR_MAX, 0), // green
	// CRGB(COLOR_MAX, 0,          0), // red
CRGB(0, COLOR_MAX, COLOR_MAX),
CRGB(0, 0, COLOR_MAX), // blue,
CRGB(COLOR_MAX, 0, COLOR_MAX)
// CRGB(COLOR_MAX, COLOR_MAX,  COLOR_MAX)
};

CRGB currentBlendedColor = colors[0];
constexpr auto s = sizeof(CRGB);
int targetColorIndex = 1;
int currStep = 0;
fract8 currLerpFrac = 255;
int color_dir = 1;

using void_ftn_ptr = void(*)(void);


Pulser myPulser;
ReversePulser revPulser;
int currentPulse = 0;

void restartForwardPulse()
{
	// Every 5 pulses, do them both :)
	if (currentPulse % 5 != 0)
	{
		revPulser.Pause();
	}
	myPulser.Start();
}

void restartRevPulser()
{
	if (currentPulse % 5 != 0)
	{
		myPulser.Pause();
	}
	revPulser.Resume();
	// revPulser.Start();
}

void setup()
{
	wait_for_serial_connection(); // Optional, but seems to help Teensy out a lot.
	// FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
	FastLED.setBrightness(BRIGHTNESS);
	// myPulser.leds = leds;
	// myPulser.Init(15, 30);
	// myPulser.Start();
	// myPulser.OnFinished(restartRevPulser);
	// revPulser.leds = leds;
	// revPulser.Init(0, 15);
	// revPulser.Start();
	// revPulser.SetHold(50);

	// revPulser.OnFinished(restartForwardPulse);
}

void targetNextColor()
{
	targetColorIndex = (targetColorIndex + 1) % 6;
}

CRGB getColorForStep()
{
	CRGB targetColor = colors[targetColorIndex];
	CRGB nextColor = CRGB::blend(currentBlendedColor, targetColor, currLerpFrac);
	currLerpFrac += color_dir;

	if (color_dir == 1 && currLerpFrac == 255)
	{
		color_dir = -1;
		targetNextColor();
	}
	else if (color_dir == -1 && currLerpFrac == 0)
	{
		color_dir = 1;
		targetNextColor();
	}

	return nextColor;
}

void pulseSegment(int startIndex, int endIndex)
{
	CRGB thisColor = getColorForStep();

	for (int i = startIndex; i < endIndex && i < NUM_LEDS; ++i)
	{
		leds[i] = thisColor;
	}
	for (int i = 0; i < startIndex; ++i)
	{
		leds[i] = CRGB::Black;
	}
	for (int i = endIndex; i < NUM_LEDS; ++i)
	{
		leds[i] = CRGB::Black;
	}
	FastLED.show();
	delay(5);
}




int currentFillUpTo = 1;
int dir = 1;

void strobeColors()
{
	CRGB color = getColorForStep();
	FastLED.showColor(color);
	delay(5);
}

int maxDelay = 25;
int minDelay = 5;
int getNextDelay(int i)
{
	int nextDelay = minDelay + int((maxDelay - minDelay) * ((double) (i) / NUM_LEDS));
	return nextDelay * 4;
}

// for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256)
uint16_t currentHue = 0;
uint16_t getNextHue()
{
	currentHue = currentHue + 1024;
	return currentHue;
}

void fillAndShow(CRGB color)
{
	for (int i = 0; i < NUM_LEDS; ++i)
	{
		leds[i] = color;
	}
	FastLED.show();
}

void loop()
{
	// // const auto frameTime = millis();
	// // const auto color = getColorForStep();
	// // FastLED.clear();
	// // FastLED.clear();
	// // myPulser.Update(color);
	// // myPulser.Show();
	// // revPulser.Update(color);
	// // revPulser.Show();
	// // FastLED.show();
	// // currentPulse++;
	// // int nextDelay = getNextDelay(currentFillUpTo);
	// // delay(nextDelay);


	uint32_t ms = millis();

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

	FastLED.show();





	// static size_t frame_count = 0;
	// int frame_cycle = frame_count % 4;
	// frame_count++;

	// CRGB pixel;
	// switch (frame_cycle)
	// {
	// 	case 0:
	// 		pixel = CRGB::Red;
	// 		break;
	// 	case 1:
	// 		pixel = CRGB::Green;
	// 		break;
	// 	case 2:
	// 		pixel = CRGB::Blue;
	// 		break;
	// 	case 3:
	// 		pixel = CRGB::White;
	// 		break;
	// }

	// for (int i = -1; i < frame_cycle; ++i)
	// {
	// 	fillAndShow(pixel);
	// 	delay(200);
	// 	fillAndShow(CRGB::Black);
	// 	delay(200);
	// }
	// delay(1000);


}
