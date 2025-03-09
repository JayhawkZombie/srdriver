#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 1

#include <FastLED.h>
#include <stdint.h>
#include "Globals.h"
#include "Behaviors/Pulser.hpp"
#include "Behaviors/ReversePulser.hpp"


CRGB leds[NUM_LEDS];



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
	myPulser.leds = leds;
	myPulser.Init(0, 63);
	myPulser.Start();
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

float easeInOutCubicFloat(float perc) {
	if (perc < 0.5) {
		return 4 * perc * perc * perc;
	} else {
		float op = -2 * perc + 2;
		return 1 - (op * op * op) / 2;
	}
}

unsigned long maxDelay = 50;
unsigned long minDelay = 5;
fract8 curr = 0;
unsigned long getNextDelay(unsigned long i)
{
	static char printbuf[256] = {0};
	// unsigned long nextDelay = minDelay + unsigned long((maxDelay - minDelay) * ((double) (i) / NUM_LEDS));
	// fract8 f = (i / NUM_LEDS) * 256;
	// uint16_t fract = (i * 65535) / NUM_LEDS;
	float fraction = i / 64.f;
	// uint16_t eased = ease8InOutCubic(fract);
	float easedFloat = easeInOutCubicFloat(fraction);
	unsigned long nextDelay = minDelay + (easedFloat * maxDelay);
	snprintf(printbuf, 256, "easedFloat: %0.4f\n", (double)easedFloat);
	Serial.write(printbuf);
	return nextDelay;
}

void loop()
{
	const auto frameTime = millis();
	const auto color = getColorForStep();
	// FastLED.clear();
	FastLED.clear();
	myPulser.Update(color);
	myPulser.Show();
	// revPulser.Update(color);
	// revPulser.Show();
	FastLED.show();
	currentPulse++;
	unsigned long nextDelay = getNextDelay(myPulser.GetCurrentIndex());
	// currentFillUpTo++;
	delay(nextDelay);
}
