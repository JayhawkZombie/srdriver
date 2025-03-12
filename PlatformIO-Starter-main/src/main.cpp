#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 1

#include <FastLED.h>
#include <stdint.h>
#include "Behaviors/Noise.hpp"
#include "Utils.hpp"
#include "Globals.h"
#include "Behaviors/Pulser.hpp"
#include "Behaviors/ReversePulser.hpp"

#include "LightPlayer2.h"

Light LightArr[NUM_LEDS];// storage for player
const unsigned int numPatterns = 7;


unsigned int patternIndex[] = { 1,2,4,5,3,6,0 };
unsigned int patternLength[] = { 64,64,64,64,64,10,30 };// taking NUM_LEDS = 64
unsigned int stepPause[] = { 1,1,2,1,1,5,1 };// crissCross and blink are slowed
unsigned int Param[] = { 3,5,1,4,1,1,1 };

LightPlayer2 LtPlay2;

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
	CRGB(COLOR_MAX, 0, 0),
	CRGB(0, COLOR_MAX, 0),
	CRGB(0, 0, COLOR_MAX),
	CRGB(COLOR_MAX, 0, COLOR_MAX)
};

Pulser myPulser;
ReversePulser revPulser;

Light onLight(200, 0, 0);// red
Light offLight(0, 0, 200);// blue

fl::FixedVector<char, 5> patternOrder;

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

	LtPlay2.init(LightArr[0], 8, 8);
	LtPlay2.setArrays(numPatterns, patternIndex, patternLength, stepPause, Param);

	patternOrder.push_back('R');
	patternOrder.push_back('D');
	patternOrder.push_back('C');
	patternOrder.push_back('Z');
	patternOrder.push_back('X');
}



unsigned long maxDelay = 505;
unsigned long minDelay = 50;
fract8 curr = 0;
// Float operations are slow on arduino, this should be using
// fixed-point arithmetic with something like fract8 (fraction of 256ths)
unsigned long getNextDelay(unsigned long i)
{
	float fraction = i / 64.f;
	float easedFloat = easeInOutCubicFloat(fraction);
	unsigned long nextDelay = minDelay + (easedFloat * maxDelay);
	return nextDelay;
}

void DrawError(const CRGB &color)
{
	for (int i = 0; i < LEDS_MATRIX_X; i += 2)
	{
		leds[i] = color;
	}
}

#include "Behaviors/Ring.hpp"
#include "Behaviors/ColumnsRows.hpp"
#include "Behaviors/Diagonals.hpp"

unsigned long lastUpdateMs = 0;
NoiseVis noise;
// int currentRing = 0;
int sharedCurrentIndexState = 0;
unsigned long last_ms = 0;
// int currentDad = 0;

// char currentPattern = 'D'; // Dad
bool moveToNextPattern = false;
int currentPatternIndex = 0;

// int currentCol = 0;
fl::FixedVector<int, LEDS_MATRIX_Y> sharedIndices;
void GoToNextPattern()
{
	// Just reset everything
	currentPatternIndex++;
	sharedCurrentIndexState = 0;
}

void UpdatePattern()
{
	const char currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	switch (currentPattern)
	{
		case 'D':
		{
			LtPlay2.update(onLight, offLight);
			for (int i = 0; i < NUM_LEDS; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 16)
			{
				GoToNextPattern();
			}
			break;
		}
		case 'R':
		{
			// Ring
			DrawRing(sharedCurrentIndexState % 4, leds, CRGB::DarkRed);
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 16)
			{
				// Go to cols
				GoToNextPattern();
			}
			break;
		}
		case 'C':
		{
			sharedIndices = GetIndicesForColumn(sharedCurrentIndexState % 8);
			DrawColumnOrRow(leds, sharedIndices, CRGB::DarkBlue);
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 16)
			{
				GoToNextPattern();
			}
			break;
		}
		case 'Z': // row
		{
			sharedIndices = GetIndicesForRow(sharedCurrentIndexState % 8);
			DrawColumnOrRow(leds, sharedIndices, CRGB::DarkGreen);
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 16)
			{
				GoToNextPattern();
			}
			break;
		}
		case 'X':
		{
			sharedIndices = GetIndicesForDiagonal(sharedCurrentIndexState % 4);
			DrawColumnOrRow(leds, sharedIndices, CRGB::SlateGray);
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 16)
			{
				GoToNextPattern();
			}
			break;
		}
		default:
		{
			DrawError(CRGB::Red);
			break;
		}
	}
}


void loop()
{
	unsigned long ms = millis();
	FastLED.clear();
	UpdatePattern();
	last_ms = ms;
	FastLED.show();
	unsigned long nextDelay = getNextDelay(myPulser.GetCurrentIndex());
	delay(nextDelay);
}
