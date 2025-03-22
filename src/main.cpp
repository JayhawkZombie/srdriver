#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 1

#include <FastLED.h>
#include <stdint.h>
#include "Behaviors/Noise.hpp"
#include "Utils.hpp"
#include "Globals.h"
#include "Behaviors/Pulser.hpp"
#include "Behaviors/ReversePulser.hpp"
#include "Behaviors/Ring.hpp"
#include "Behaviors/ColumnsRows.hpp"
#include "Behaviors/Diagonals.hpp"
#include "LightPlayer2.h"

#include <Utils.hpp>



Light LightArr[NUM_LEDS];// storage for player
const unsigned int numPatterns = 7;


unsigned int patternIndex[] = { 1,2,4,5,3,6,0 };
unsigned int patternLength[] = { 64,64,64,64,64,10,30 };// taking NUM_LEDS = 64
unsigned int stepPause[] = { 1,1,2,1,1,5,1 };// crissCross and blink are slowed
unsigned int Param[] = { 3,5,1,4,1,1,1 };

// LightPlayer2 LtPlay2;

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

// storage for the procedural patterns
patternData pattData[16];// each procedural pattern once + pattern #100 once

// storage for a 3 step pattern #100
uint8_t stateData[24];// enough for 24*8 = 192 = 3*64 state assignments

// LightPlayer2 uses 
Light onLt(200, 0, 60);// these
Light offLt(60, 0, 200);// Lights

fl::FixedVector<char, 5> patternOrder;
LightPlayer2 LtPlay2;

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

	pattData[0].init(1, 1, 5);
	pattData[1].init(2, 1, 3);
	pattData[2].init(7, 8, 10);//  checkerboard blink
	pattData[3].init(100, 20, 1);// pattern 100 persists for 20 frames
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
	pattData[15].init(0, 30, 1);// 30 x loop() calls pause before replay

	LtPlay2.init(LightArr[0], 8, 8, pattData[0], 15);

	// explicit assignment of each Byte = 1 row of Lights
	// tall rectangle
	stateData[0] = 60;
	stateData[1] = 36;
	stateData[2] = 36;
	stateData[3] = 36;
	stateData[4] = 36;
	stateData[5] = 36;
	stateData[6] = 36;
	stateData[7] = 60;
	// centered box
	stateData[8] = 0;
	stateData[9] = 0;
	stateData[10] = 60;
	stateData[11] = 60;
	stateData[12] = 60;
	stateData[13] = 60;
	stateData[14] = 0;
	stateData[15] = 0;
	// wide rectangle
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



unsigned long maxDelay = 505;
unsigned long minDelay = 50;
fract8 curr = 0;
unsigned long getNextDelay(unsigned long i)
{
	return InterpolateCubicFloat(minDelay, maxDelay, i / 64.f);
}

void DrawError(const CRGB &color)
{
	for (int i = 0; i < LEDS_MATRIX_X; i += 2)
	{
		leds[i] = color;
	}
}

unsigned long lastUpdateMs = 0;
NoiseVis noise;
int sharedCurrentIndexState = 0;
unsigned long last_ms = 0;
bool moveToNextPattern = false;
int currentPatternIndex = 0;

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
			LtPlay2.update(onLt, offLt);
			for (int i = 0; i < NUM_LEDS; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 100)
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
