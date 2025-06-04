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

LightPlayer2 LtPlay2; // Declare the LightPlayer2 instance
LightPlayer2 LtPlayJewel; // Declare the LightPlayer2 instance
LightPlayer2 LtPlayStrip2; // Declare the LightPlayer2 instance

// storage for the procedural patterns
patternData pattData[16];// each procedural pattern once + pattern #100 once
patternData pattDataJewel[16];
patternData pattDataStrip2[16];

// storage for a 3 step pattern #100
uint8_t stateData[24];// enough for 24*8 = 192 = 3*64 state assignments

// LightPlayer2 uses 
Light onLt(200, 0, 60);// these
Light offLt(60, 0, 200);// Lights

#include "die.hpp"
#include "WavePlayer.h"
WavePlayer wavePlayer;
WavePlayer wavePlayer2;
WavePlayer wavePlayer3;
WavePlayer wavePlayer4;
DataPlayer dataPlayer;
float C_Rt[3] = { 3, 2, 1 };

CRGB arr[16 * 16];
DataPlayer dp;

extern void initDataPlayer(DataPlayer &dp, uint8_t *data, uint16_t numData, Light *arr);
extern void initWaveData(WavePlayer &wp, Light *arr);
extern void initWaveData2(WavePlayer &wp, Light *arr);
extern void initWaveData3(WavePlayer &wp, Light *arr);
extern void initWaveData4(WavePlayer &wp, Light *arr);

enum class PatternType
{
	DADS_PATTERN_PLAYER,
	RING_PATTERN,
	COLUMN_PATTERN,
	ROW_PATTERN,
	DIAGONAL_PATTERN,
	WAVE_PLAYER1_PATTERN,
	WAVE_PLAYER2_PATTERN,
	WAVE_PLAYER3_PATTERN,
	WAVE_PLAYER4_PATTERN,
	DATA_PATTERN,
};

fl::FixedVector<PatternType, 20> patternOrder;

void setup()
{
	wait_for_serial_connection(); // Optional, but seems to help Teensy out a lot.
	// Used for RGB (NOT RGBW) LED strip
	#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
		FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
	#else
		FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	#endif
	// Used for RGBW (ring/string/matrix)
	// FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
	FastLED.setBrightness(BRIGHTNESS);
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);

	LtPlay2.onLt = Light(0, 255, 255);
	LtPlay2.offLt = Light(0, 0, 0);
	LtPlayJewel.onLt = Light(0, 255, 255);
	LtPlayJewel.offLt = Light(0, 0, 0);
	LtPlayStrip2.onLt = Light(0, 255, 255);
	LtPlayStrip2.offLt = Light(0, 0, 0);
	Serial.println("Setup");

	// patternOrder.push_back('R');
	// patternOrder.push_back('W');
	patternOrder.push_back(PatternType::WAVE_PLAYER1_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER2_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER3_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER4_PATTERN);
	// patternOrder.push_back('Q');
	// patternOrder.push_back(PatternType::DADS_PATTERN_PLAYER);
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

	// Randomize the pattern order in patternData
	for (int i = 0; i < 16; ++i)
	{
		if (pattData[i].funcIndex != 100) {
			pattData[i].funcIndex = random(0, 16);
		}
	}

	pattDataJewel[0].init(1, 1, 5);
	pattDataJewel[1].init(2, 1, 3);
	pattDataJewel[2].init(7, 8, 10); // checkerboard blink
	pattDataJewel[3].init(100, 20, 1); // pattern 100 persists for 20 frames
	pattDataJewel[4].init(3, 1, 1);
	pattDataJewel[5].init(4, 1, 1);
	pattDataJewel[6].init(5, 1, 3);
	pattDataJewel[7].init(6, 8, 12);
	pattDataJewel[8].init(10, 2, 1);
	pattDataJewel[9].init(11, 2, 1);
	pattDataJewel[10].init(12, 2, 1);
	pattDataJewel[11].init(13, 2, 1);
	pattDataJewel[12].init(14, 4, 1);
	pattDataJewel[13].init(15, 4, 1);
	pattDataJewel[14].init(16, 2, 1);
	pattDataJewel[15].init(0, 30, 1); // 30 x loop() calls pause before replay
	for (int i = 0; i < 16; ++i)
	{
		if (pattDataJewel[i].funcIndex != 100) {
			pattDataJewel[i].funcIndex = random(0, 16);
		}
	}

	pattDataStrip2[0].init(1, 1, 5);
	pattDataStrip2[1].init(2, 1, 3);
	pattDataStrip2[2].init(7, 8, 10); // checkerboard blink
	pattDataStrip2[3].init(100, 20, 1); // pattern 100 persists for 20 frames
	pattDataStrip2[4].init(3, 1, 1);
	pattDataStrip2[5].init(4, 1, 1);
	pattDataStrip2[6].init(5, 1, 3);
	pattDataStrip2[7].init(6, 8, 12);
	pattDataStrip2[8].init(10, 2, 1);
	pattDataStrip2[9].init(11, 2, 1);
	pattDataStrip2[10].init(12, 2, 1);
	pattDataStrip2[11].init(13, 2, 1);
	pattDataStrip2[12].init(14, 4, 1);
	pattDataStrip2[13].init(15, 4, 1);
	pattDataStrip2[14].init(16, 2, 1);
	pattDataStrip2[15].init(0, 30, 1); // 30 x loop() calls pause before replay
	for (int i = 0; i < 16; ++i)
	{
		if (pattDataStrip2[i].funcIndex != 100) {
			pattDataStrip2[i].funcIndex = random(0, 16);
		}
	}

	// Initialize LightPlayer2
	LtPlay2.init(LightArr[LEDS_STRIP_1_START], 1, LEDS_STRIP_SHORT, pattData[0], 15);
	LtPlayJewel.init(LightArr[LEDS_JEWEL_START], 1, LEDS_JEWEL, pattDataJewel[0], 15);
	LtPlayStrip2.init(LightArr[LEDS_STRIP_2_START], 1, LEDS_STRIP_SHORT, pattDataStrip2[0], 15);
	LtPlay2.update();
	LtPlayJewel.update();
	LtPlayStrip2.update();

	initWaveData(wavePlayer, LightArr);
	initWaveData2(wavePlayer2, LightArr);
	initWaveData3(wavePlayer3, LightArr);
	initWaveData4(wavePlayer4, LightArr);

	// wavePlayer.init(LightArr[0], LEDS_MATRIX_X, LEDS_MATRIX_Y, Light(255, 255, 255), Light(0, 0, 0));
	// wavePlayer.setWaveData(1.1f, 16.f, 16.f, 9.f, 64.f);
	// wavePlayer.update(0.f);
	// wavePlayer.setSeriesCoeffs(C_Rt, 3, nullptr, 0);
	// wavePlayer.AmpLt = 1.734f;
	// wavePlayer.AmpRt = 0.405f;
	// wavePlayer.wvLenLt = 42.913f;
	// wavePlayer.wvLenRt = 99.884f;
	// wavePlayer.wvSpdLt = 83.607f;
	// wavePlayer.wvSpdRt = 17.757f;

	// initDataPlayer(dp, dv, 1024, LightArr);
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
	const auto currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	switch (currentPattern)
	{
		case PatternType::DADS_PATTERN_PLAYER:
		{
			LtPlay2.update();
			for (int i = 0; i < NUM_LEDS; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 300)
			{
				GoToNextPattern();
			}
			break;
		}
		case PatternType::RING_PATTERN:
		{
			// Ring
			DrawRing(sharedCurrentIndexState % 4, leds, CRGB::DarkRed);
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 160)
			{
				// Go to cols
				GoToNextPattern();
			}
			break;
		}
		case PatternType::COLUMN_PATTERN:
		{
			sharedIndices = GetIndicesForColumn(sharedCurrentIndexState % 8);
			DrawColumnOrRow(leds, sharedIndices, CRGB::DarkBlue);
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 160)
			{
				GoToNextPattern();
			}
			break;
		}
		case PatternType::ROW_PATTERN:
		{
			sharedIndices = GetIndicesForRow(sharedCurrentIndexState % 8);
			DrawColumnOrRow(leds, sharedIndices, CRGB::DarkGreen);
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 160)
			{
				GoToNextPattern();
			}
			break;
		}
		case PatternType::DIAGONAL_PATTERN:
		{
			sharedIndices = GetIndicesForDiagonal(sharedCurrentIndexState % 4);
			DrawColumnOrRow(leds, sharedIndices, CRGB::SlateGray);
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 160)
			{
				GoToNextPattern();
			}
			break;
		}
		case PatternType::WAVE_PLAYER1_PATTERN:
		{
			Serial.println("WavePlayer1");
			wavePlayer.update(0.01f);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
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
		case PatternType::WAVE_PLAYER2_PATTERN:
		{
			wavePlayer2.update(0.01f);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
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
		case PatternType::WAVE_PLAYER3_PATTERN:
		{
			wavePlayer3.update(0.01f);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
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

		case PatternType::WAVE_PLAYER4_PATTERN:
		{
			wavePlayer4.update(0.01f);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
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
		case PatternType::DATA_PATTERN:
		{
			wavePlayer.update(0.03f);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			dp.drawOff = false;
			dp.update();
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				// if (LightArr[i].r == 255 && LightArr[i].g == 255 && LightArr[i].b == 255) {
				// 	continue;
				// }
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			sharedCurrentIndexState++;
			if (sharedCurrentIndexState >= 300)
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


	// Also always update the strip short
	LtPlay2.update();
	LtPlayJewel.update();
	LtPlayStrip2.update();
	for (int i = 0; i < NUM_LEDS; ++i) {
		leds[i].r = LightArr[i].r;
		leds[i].g = LightArr[i].g;
		leds[i].b = LightArr[i].b;
	}
	// for (int i = 0; i < LEDS_STRIP_SHORT; ++i)
	// {
	// 	leds[i + LEDS_STRIP_1_START].r = LightArr[i].r;
	// 	leds[i + LEDS_STRIP_1_START].g = LightArr[i].g;
	// 	leds[i + LEDS_STRIP_1_START].b = LightArr[i].b;
	// }
	// for (int i = 0; i < LEDS_JEWEL; ++i)
	// {
	// 	leds[i + LEDS_JEWEL_START].r = LightArr[i].r;
	// 	leds[i + LEDS_JEWEL_START].g = LightArr[i].g;
	// 	leds[i + LEDS_JEWEL_START].b = LightArr[i].b;
	// }
	// for (int i = 0; i < LEDS_STRIP_SHORT; ++i)
	// {
	// 	leds[i + LEDS_STRIP_2_START].r = LightArr[i].r;
	// 	leds[i + LEDS_STRIP_2_START].g = LightArr[i].g;
	// 	leds[i + LEDS_STRIP_2_START].b = LightArr[i].b;
	// }
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
	delay(15.f);
}
