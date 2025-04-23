#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0

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
#include "DataPlayer.h"

#include <Utils.hpp>

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

Pulser myPulser;

Light LightArr[NUM_LEDS];// storage for player
const unsigned int numPatterns = 7;

unsigned int patternIndex[] = { 1,2,4,5,3,6,0 };
unsigned int patternLength[] = { 64,64,64,64,64,10,30 };// taking NUM_LEDS = 64
unsigned int stepPause[] = { 1,1,2,1,1,5,1 };// crissCross and blink are slowed
unsigned int Param[] = { 3,5,1,4,1,1,1 };

CRGB leds[NUM_LEDS];
CRGB ledNoise[NUM_LEDS];
uint8_t dv[1024];
fl::FixedVector<char, 10> patternOrder;

LightPlayer2 LtPlay2; // Declare the LightPlayer2 instance

// storage for the procedural patterns
patternData pattData[16];// each procedural pattern once + pattern #100 once

// storage for a 3 step pattern #100
uint8_t stateData[24];// enough for 24*8 = 192 = 3*64 state assignments

// LightPlayer2 uses 
Light onLt(200, 0, 60);// these
Light offLt(60, 0, 200);// Lights

#include "die.hpp"
#include "WavePlayer.h"
WavePlayer wavePlayer;
DataPlayer dataPlayer;
float C_Rt[3] = { 3, 2, 1 };

CRGB arr[16 * 16];
DataPlayer dp;

extern void initDataPlayer(DataPlayer &dp, uint8_t *data, uint16_t numData, Light *arr);

void setup()
{
	wait_for_serial_connection(); // Optional, but seems to help Teensy out a lot.
	// Used for RGB (NOT RGBW) LED strip
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	// Used for RGBW (ring/string/matrix)
	// FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
	FastLED.setBrightness(BRIGHTNESS);
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);

	LtPlay2.onLt = Light(255, 255, 255);
	LtPlay2.offLt = Light(0, 0, 0);

	// patternOrder.push_back('R');
	patternOrder.push_back('W');
	// patternOrder.push_back('Q');
	// patternOrder.push_back('D');
	// patternOrder.push_back('C');
	// patternOrder.push_back('Z');
	// patternOrder.push_back('X');

	pattData[0].init(1, 1, 5);
	pattData[1].init(2, 1, 3);
	pattData[2].init(7, 8, 10); // checkerboard blink
	pattData[3].init(100, 20, 1); // pattern 100 persists for 20 frames
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
	pattData[15].init(0, 30, 1); // 30 x loop() calls pause before replay

	// Initialize LightPlayer2
	LtPlay2.init(*LightArr, 8, 8, pattData[0], 15);
	LtPlay2.update();

	wavePlayer.init(LightArr[0], LEDS_MATRIX_X, LEDS_MATRIX_Y, Light(19, 0, 54), Light(0, 0, 0));
	wavePlayer.setWaveData(1.1f, 64.f, 64.f, 9.f, 64.f);
	wavePlayer.update(0.f);
	wavePlayer.setSeriesCoeffs(C_Rt, 3, nullptr, 0);
	// wavePlayer.AmpLt = 1.734f;
	// wavePlayer.AmpRt = 0.405f;
	// wavePlayer.wvLenLt = 42.913f;
	// wavePlayer.wvLenRt = 99.884f;
	// wavePlayer.wvSpdLt = 83.607f;
	// wavePlayer.wvSpdRt = 17.757f;

	initDataPlayer(dp, dv, 1024, LightArr);
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
int sharedCurrentIndexState = 0;
unsigned long last_ms = 0;
int currentPatternIndex = 0;

fl::FixedVector<int, LEDS_MATRIX_Y> sharedIndices;
void GoToNextPattern()
{
	// Just reset everything
	currentPatternIndex++;
	sharedCurrentIndexState = 0;
	Serial.println("GoToNextPattern" + String(currentPatternIndex));
}

void UpdatePattern()
{
	const char currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	switch (currentPattern)
	{
		case 'D':
		{
			LtPlay2.update();
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
		case 'W':
		{
			wavePlayer.update(0.01f);
			for (int i = 0; i < NUM_LEDS; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 64)
			{
				GoToNextPattern();
			}
			break;
		}
		case 'Q':
		{
			// dp.update();
			// for (int i = 0; i < NUM_LEDS; ++i)
			// {
			// 	leds[i].r = LightArr[i].r;
			// 	leds[i].g = LightArr[i].g;
			// 	leds[i].b = LightArr[i].b;
			// }
			// sharedCurrentIndexState++;
			// if (sharedCurrentIndexState >= 100)
			// {
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

int loopCount = 0;
void loop()
{
	unsigned long ms = millis();
	FastLED.clear();
	UpdatePattern();
	last_ms = ms;
	FastLED.show();
	unsigned long nextDelay = getNextDelay(myPulser.GetCurrentIndex());
	delay(5.f);
}
