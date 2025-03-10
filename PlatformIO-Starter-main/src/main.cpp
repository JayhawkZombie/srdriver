#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 1

#include <FastLED.h>
#include <stdint.h>
#include "Behaviors/Noise.hpp"
#include "Globals.h"
#include "Behaviors/Pulser.hpp"
#include "Behaviors/ReversePulser.hpp"

CRGB leds[NUM_LEDS];
CRGB ledNoise[NUM_LEDS];

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
	// CRGB(COLOR_MAX, COLOR_MAX,  0),
	CRGB(0, COLOR_MAX, 0), // green
	// CRGB(COLOR_MAX, 0,          0), // red
	// CRGB(0, COLOR_MAX, COLOR_MAX),
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
	// Used for RGB (NOT RGBW) LED strip
	// FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	// Used for RGBW (ring/string/matrix)
	FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
	FastLED.setBrightness(BRIGHTNESS);
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);
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
// Float operations are slow on arduino, this should be using
// fixed-point arithmetic with something like fract8 (fraction of 256ths)
unsigned long getNextDelay(unsigned long i)
{
	static char printbuf[256] = {0};
	float fraction = i / 64.f;
	float easedFloat = easeInOutCubicFloat(fraction);
	unsigned long nextDelay = minDelay + (easedFloat * maxDelay);
	return nextDelay;
}

unsigned long targetElapsed = 5000;
unsigned long currentElapsed = 0;

int getIndexForElapsed()
{
	float fract = currentElapsed * 1.f / targetElapsed;
	float interp = easeInOutCubicFloat(fract);
	return interp * NUM_LEDS;
}

int currentIndex = 0;
void fillTo(int index, CRGB color) {
	for (int i = 0; i < index; ++i) {
		leds[i] = color;
	}
	for (int i = index; i < NUM_LEDS; ++i) {
		leds[i] = CRGB::Black;
	}
}

unsigned long lastUpdateMs = 0;
NoiseVis noise;

void loop()
{
	unsigned long ms = millis();
	// currentElapsed += 50;
	// int thisIndex = getIndexForElapsed();
	const auto color = getColorForStep();
	// FastLED.clear();
	// fillTo(thisIndex, color);
	// if (currentElapsed >= targetElapsed) {
	// 	currentElapsed = 0;
	// }
	myPulser.Update(color);
	myPulser.Show();
	revPulser.Update(color);
	revPulser.Show();

	// Want to apply noise on top of the fill
	// For all that aren't black, fill with the noise
	noise.Update(ms, ledNoise);
	for (int i = 0; i < NUM_LEDS; ++i) {
		if (leds[i] == CRGB::Black) {
			continue;
		}
		leds[i] = ledNoise[i];
	}

	FastLED.show();
	currentPulse++;
	// lastUpdateMs = ms;
	unsigned long nextDelay = getNextDelay(myPulser.GetCurrentIndex());
	delay(nextDelay);
}
