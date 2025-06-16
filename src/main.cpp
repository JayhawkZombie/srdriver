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
#include <Adafruit_NeoPixel.h>
#include <Utils.hpp>
#include "hal/buttons.hpp"
#include "hal/potentiometer.hpp"

#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
Rgbw rgbw = Rgbw(
	kRGBWDefaultColorTemp,
	kRGBWExactColors,      // Mode
	W3                     // W-placement
);

typedef WS2812<LED_PIN, RGB> ControllerT;  // RGB mode must be RGB, no re-ordering allowed.
static RGBWEmulatedController<ControllerT, GRB> rgbwEmu(rgbw);  // ordering goes here.
#endif

void wait_for_serial_connection()
{
	uint32_t timeout_end = millis() + 2000;
	Serial.begin(9600);
	while (!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
}

Light LightArr[NUM_LEDS];// storage for player
CRGB leds[NUM_LEDS];

LightPlayer2 LtPlay2; // Declare the LightPlayer2 instance
LightPlayer2 LtPlay3; // Declare the LightPlayer2 instance
LightPlayer2 LtPlayJewel; // Declare the LightPlayer2 instance
LightPlayer2 LtPlayStrip;
LightPlayer2 LtPlayStrip2; // Declare the LightPlayer2 instance

// storage for the procedural patterns
patternData pattData[16];// each procedural pattern once + pattern #100 once
patternData pattData3[16];// each procedural pattern once + pattern #100 once
patternData pattDataJewel[16];
patternData pattDataStrip[16];
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
WavePlayer wavePlayer5;
WavePlayer wavePlayer6;
WavePlayer wavePlayer7;
DataPlayer dataPlayer;

int wavePlayerLengths[7] = { 100, 100, 100, 300, 300, 300, 300 };
float wavePlayerSpeeds[7] = { 0.01f, 0.035f, 0.03f, 0.01f, 0.01f, 0.01f, 0.01f };

DataPlayer dp;

extern void initDataPlayer(DataPlayer &dp, uint8_t *data, uint16_t numData, Light *arr);
extern void initWaveData(WavePlayer &wp, Light *arr);
extern void initWaveData2(WavePlayer &wp, Light *arr);
extern void initWaveData3(WavePlayer &wp, Light *arr);
extern void initWaveData4(WavePlayer &wp, Light *arr);
extern void initWaveData5(WavePlayer &wp, Light *arr);
extern void initWaveData6(WavePlayer &wp, Light *arr);
extern void initWaveData7(WavePlayer &wp, Light *arr);

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
	WAVE_PLAYER5_PATTERN,
	WAVE_PLAYER6_PATTERN,
	WAVE_PLAYER7_PATTERN,
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
	FastLED.setBrightness(BRIGHTNESS);
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);

	LtPlay2.onLt = Light(0, 255, 255);
	LtPlay2.offLt = Light(0, 0, 0);
	LtPlay3.onLt = Light(235, 0, 52);
	LtPlay3.offLt = Light(0, 0, 0);
	LtPlayJewel.onLt = Light(0, 255, 0);
	LtPlayJewel.offLt = Light(0, 0, 0);
	LtPlayStrip.onLt = Light(179, 255, 0);
	LtPlayStrip.offLt = Light(0, 0, 0);
	LtPlayStrip2.onLt = Light(189, 0, 9);
	LtPlayStrip2.offLt = Light(0, 0, 0);
	Serial.println("Setup");

	patternOrder.push_back(PatternType::WAVE_PLAYER1_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER2_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER3_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER4_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER5_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER6_PATTERN);
	patternOrder.push_back(PatternType::WAVE_PLAYER7_PATTERN);
	pattData[0].init(33, 3, 1279);
	pattData[1].init(34, 3, 1279);
	// pattData[1].init(32, 2, 2243);
	pattData[2].init(0, 30, 1); // 30 x loop() calls pause before replay

	pattData3[0].init(33, 3, 536);
	pattData3[1].init(34, 3, 536);
	pattData3[2].init(33, 3, 792);
	pattData3[3].init(34, 3, 792);
	pattData3[4].init(33, 3, 1816);
	pattData3[5].init(34, 3, 1816);
	pattData3[6].init(33, 3, 280);
	pattData3[7].init(34, 3, 280);
	pattData3[8].init(33, 3, 536);
	pattData3[9].init(34, 3, 536);
	pattData3[10].init(0, 30, 1); // 30 x loop() calls pause before replay

	pattDataJewel[0].init(1, 6, 5);
	pattDataJewel[1].init(2, 6, 3);
	// pattDataJewel[2].init(7, 8, 10); // checkerboard blink
	// pattDataJewel[3].init(100, 20, 1); // pattern 100 persists for 20 frames
	// pattDataJewel[4].init(3, 1, 1);
	// pattDataJewel[5].init(4, 1, 1);
	// pattDataJewel[6].init(5, 1, 3);
	// pattDataJewel[7].init(6, 8, 12);
	// pattDataJewel[8].init(10, 2, 1);
	// pattDataJewel[9].init(11, 2, 1);
	// pattDataJewel[10].init(12, 2, 1);
	// pattDataJewel[11].init(13, 2, 1);
	// pattDataJewel[12].init(14, 4, 1);
	// pattDataJewel[13].init(15, 4, 1);
	// pattDataJewel[14].init(16, 2, 1);
	pattDataJewel[15].init(0, 30, 1); // 30 x loop() calls pause before replay
	// for (int i = 0; i < 16; ++i)
	// {
	// 	if (pattDataJewel[i].funcIndex != 100)
	// 	{
	// 		// pattDataJewel[i].funcIndex = random(0, 16);
	// 	}
	// }

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
		if (pattDataStrip2[i].funcIndex != 100)
		{
			pattDataStrip2[i].funcIndex = random(0, 16);
		}
	}

	pattDataStrip[0].init(1, 1, 5);
	pattDataStrip[1].init(2, 1, 3);
	pattDataStrip[2].init(7, 8, 10); // checkerboard blink
	pattDataStrip[3].init(100, 20, 1); // pattern 100 persists for 20 frames
	pattDataStrip[4].init(3, 1, 1);
	pattDataStrip[5].init(4, 1, 1);
	pattDataStrip[6].init(5, 1, 3);
	pattDataStrip[7].init(6, 8, 12);
	pattDataStrip[8].init(10, 2, 1);
	pattDataStrip[9].init(11, 2, 1);
	pattDataStrip[10].init(12, 2, 1);
	pattDataStrip[11].init(13, 2, 1);
	pattDataStrip[12].init(14, 4, 1);
	pattDataStrip[13].init(15, 4, 1);
	pattDataStrip[14].init(16, 2, 1);
	pattDataStrip[15].init(0, 30, 1); // 30 x loop() calls pause before replay
	for (int i = 0; i < 16; ++i)
	{
		if (pattDataStrip[i].funcIndex != 100)
		{
			pattDataStrip[i].funcIndex = random(0, 16);
		}
	}

	// Initialize LightPlayer2
	LtPlay2.init(LightArr[0], 8, 8, pattData[0], 2);
	LtPlay3.init(LightArr[0], 8, 8, pattData3[0], 4);
	LtPlayJewel.init(LightArr[LEDS_JEWEL_START], 1, LEDS_JEWEL, pattDataJewel[0], 3);
	LtPlayStrip.init(LightArr[LEDS_STRIP_1_START], 1, LEDS_STRIP_SHORT, pattDataStrip[0], 15);
	LtPlayStrip2.init(LightArr[LEDS_STRIP_2_START], 1, LEDS_STRIP_SHORT, pattDataStrip2[0], 15);
	LtPlay2.update();
	LtPlay3.update();
	LtPlayJewel.update();
	LtPlayStrip.update();
	LtPlayStrip2.update();
	initWaveData(wavePlayer, LightArr);
	initWaveData2(wavePlayer2, LightArr);
	initWaveData3(wavePlayer3, LightArr);
	initWaveData4(wavePlayer4, LightArr);
	initWaveData5(wavePlayer5, LightArr);
	initWaveData6(wavePlayer6, LightArr);
	initWaveData7(wavePlayer7, LightArr);
	pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
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
float speedMultiplier = 1.0f;

fl::FixedVector<int, LEDS_MATRIX_Y> sharedIndices;
void GoToNextPattern()
{
	// Just reset everything
	currentPatternIndex++;
	sharedCurrentIndexState = 0;
	Serial.println("GoToNextPattern" + String(currentPatternIndex));
}

void IncrementSharedCurrentIndexState(unsigned int limit, unsigned int count = 1)
{
	sharedCurrentIndexState += count;
	if (!ONLY_PUSHBUTTON_PATTERN_CHANGE && sharedCurrentIndexState >= limit)
	{
		GoToNextPattern();
	}
}

void UpdatePattern()
{
	ButtonEvent buttonEvent = GetButtonEvent();
	
	if (buttonEvent == ButtonEvent::PRESS)
	{
		GoToNextPattern();
	}
	else if (buttonEvent == ButtonEvent::HOLD)
	{
		speedMultiplier += 1.f;
		// Reset to 1 if it's over 10
		if (speedMultiplier > 10.f)
		{
			speedMultiplier = 1.f;
		}
		Serial.println("Speed multiplier: " + String(speedMultiplier));
	}

	for (int i = 0; i < NUM_LEDS; ++i)
	{
		LightArr[i].r = 0;
		LightArr[i].g = 0;
		LightArr[i].b = 0;
	}

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
			IncrementSharedCurrentIndexState(300);
			break;
		}
		case PatternType::RING_PATTERN:
		{
			// Ring
			DrawRing(sharedCurrentIndexState % 4, leds, CRGB::DarkRed);
			IncrementSharedCurrentIndexState(160);
			break;
		}
		case PatternType::COLUMN_PATTERN:
		{
			sharedIndices = GetIndicesForColumn(sharedCurrentIndexState % 8);
			DrawColumnOrRow(leds, sharedIndices, CRGB::DarkBlue);
			IncrementSharedCurrentIndexState(160);
			break;
		}
		case PatternType::ROW_PATTERN:
		{
			sharedIndices = GetIndicesForRow(sharedCurrentIndexState % 8);
			DrawColumnOrRow(leds, sharedIndices, CRGB::DarkGreen);
			IncrementSharedCurrentIndexState(160);
			break;
		}
		case PatternType::DIAGONAL_PATTERN:
		{
			sharedIndices = GetIndicesForDiagonal(sharedCurrentIndexState % 4);
			DrawColumnOrRow(leds, sharedIndices, CRGB::SlateGray);
			IncrementSharedCurrentIndexState(160);
			break;
		}
		case PatternType::WAVE_PLAYER1_PATTERN:
		{
			wavePlayer.update(wavePlayerSpeeds[0] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[0]);
			break;
		}
		case PatternType::WAVE_PLAYER2_PATTERN:
		{
			wavePlayer2.update(wavePlayerSpeeds[1] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[1]);
			break;
		}
		case PatternType::WAVE_PLAYER3_PATTERN:
		{
			wavePlayer3.update(wavePlayerSpeeds[2] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[2]);
			break;
		}

		case PatternType::WAVE_PLAYER4_PATTERN:
		{
			wavePlayer4.update(wavePlayerSpeeds[3] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[3]);
			break;
		}
		case PatternType::WAVE_PLAYER5_PATTERN:
		{
			wavePlayer5.update(wavePlayerSpeeds[4] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[4]);
			break;
		}
		case PatternType::WAVE_PLAYER6_PATTERN:
		{
			wavePlayer6.update(wavePlayerSpeeds[5] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[5]);
			break;
		}
		case PatternType::WAVE_PLAYER7_PATTERN:
		{
			wavePlayer7.update(wavePlayerSpeeds[6] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[6]);
			break;
		}
		case PatternType::DATA_PATTERN:
		{
			wavePlayer.update(wavePlayerSpeeds[0]);
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
			IncrementSharedCurrentIndexState(300);
			break;
		}
		default:
		{
			DrawError(CRGB::Red);
			break;
		}
	}



	// LtPlay2.updateOnOnly();
	// LtPlay3.updateOnOnly();
	LtPlayJewel.updateOnOnly();
	LtPlayStrip.updateOnOnly();
	LtPlayStrip2.updateOnOnly();


	// Also always update the strip short
	// if (sharedCurrentIndexState % 2 == 0)
	// {
	// 	LtPlay3.updateOnOnly();
	// }
	// if (sharedCurrentIndexState % 3 == 0)
	// {
	// 	LtPlayJewel.updateOnOnly();
	// }
	// if (sharedCurrentIndexState % 4 == 0)
	// {
	// 	LtPlayStrip.updateOnOnly();
	// 	LtPlayStrip2.updateOnOnly();
	// }
	for (int i = 0; i < NUM_LEDS; ++i)
	{
		leds[i].r = LightArr[i].r;
		leds[i].g = LightArr[i].g;
		leds[i].b = LightArr[i].b;
	}
}

void CheckPotentiometers()
{
	// 12-bit ADC, so we get 0-4095 instead of 0-1023
	int brightness = GetMappedPotentiometerValue(0, 255, 4095);
	FastLED.setBrightness(brightness);
}

int loopCount = 0;
void loop()
{
	unsigned long ms = millis();
	FastLED.clear();
	UpdatePattern();
	CheckPotentiometers();
	last_ms = ms;
	FastLED.show();
	delay(64.f);
}
