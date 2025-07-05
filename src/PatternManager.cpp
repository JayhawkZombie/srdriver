#include "PatternManager.h"
#include <FastLED.h>
#include "Globals.h"
#include "Light.h"
#include "DeviceState.h"
#include <array>

// Add externs for all globals and helpers used by pattern logic
unsigned int findAvailablePatternPlayer();

// Pattern-related global definitions
std::array<PatternType, 20> patternOrder;
size_t patternOrderSize = 0;
int currentWavePlayerIndex = 0;
int currentPatternIndex = 0;
std::array<patternData, 40> lp2Data;
std::array<LightPlayer2, 40> firedPatternPlayers;
WavePlayerConfig wavePlayerConfigs[10];
Light LightArr[NUM_LEDS];
Button pushButton(PUSHBUTTON_PIN);
Button pushButtonSecondary(PUSHBUTTON_PIN_SECONDARY);
float wavePlayerSpeeds[] = { 0.001f, 0.0035f, 0.003f, 0.001f, 0.001f, 0.0005f, 0.001f, 0.001f, 0.001f, 0.001f };
int wavePlayerLengths[] = { 100, 100, 100, 300, 300, 300, 300, 300, 300, 100 };
DataPlayer dp;
WavePlayer wavePlayer;
fl::FixedVector<int, LEDS_MATRIX_Y> sharedIndices;

// --- Pattern Logic Isolation ---
void Pattern_Setup() {
    patternOrderSize = 0;
    patternOrder[patternOrderSize++] = PatternType::WAVE_PLAYER_PATTERN;
    initWaveData(wavePlayerConfigs[0]);
    initWaveData2(wavePlayerConfigs[1]);
    initWaveData3(wavePlayerConfigs[2]);
    initWaveData4(wavePlayerConfigs[3]);
    initWaveData5(wavePlayerConfigs[4]);
    initWaveData6(wavePlayerConfigs[5]);
    initWaveData7(wavePlayerConfigs[6]);
    initWaveData8(wavePlayerConfigs[7]);
    initWaveData9(wavePlayerConfigs[8]);
    initWaveData10(wavePlayerConfigs[9]);
    SwitchWavePlayerIndex(0);
    for (auto &player : firedPatternPlayers) {
        player.onLt = Light(255, 255, 255);
        player.offLt = Light(0, 0, 0);
        player.init(LightArr[0], 1, 120, lp2Data[0], 18);
        player.drawOffLt = false;
        player.setToPlaySinglePattern(true);
        player.update();
    }
}

void Pattern_Loop() {
    UpdatePattern(pushButton.getEvent());
    FastLED.show();
}

void Pattern_HandleBLE(const String& characteristic, const String& value) {
    if (characteristic == "patternIndex") {
        GoToPattern(value.toInt());
    } else if (characteristic == "highColor") {
        // Parse and update color
    }
    // ...etc...
}

void Pattern_FireSingle(int idx, Light on, Light off) {
    FirePatternFromBLE(idx, on, off);
}
// --- End Pattern Logic Isolation ---

void SwitchWavePlayerIndex(int index)
{
	const auto config = wavePlayerConfigs[index];
	wavePlayer.nTermsLt = wavePlayer.nTermsRt = 0;
	wavePlayer.C_Lt = wavePlayer.C_Rt = nullptr;
	wavePlayer.init(LightArr[0], config.rows, config.cols, config.onLight, config.offLight);
	wavePlayer.setWaveData(config.AmpRt, config.wvLenLt, config.wvSpdLt, config.wvLenRt, config.wvSpdRt);
	if (config.useLeftCoefficients || config.useRightCoefficients) {
		wavePlayer.setSeriesCoeffs_Unsafe(config.C_Rt, config.nTermsRt, config.C_Lt, config.nTermsLt);
	}
}

void GoToNextPattern()
{
	const auto currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	currentWavePlayerIndex++;
	if (currentWavePlayerIndex >= numWavePlayerConfigs)
	{
		currentWavePlayerIndex = 0;
	}
	if (currentPattern == PatternType::WAVE_PLAYER_PATTERN)
	{
		SwitchWavePlayerIndex(currentWavePlayerIndex);
	}
	sharedCurrentIndexState = 0;
	Serial.println("GoToNextPattern" + String(currentWavePlayerIndex));
	UpdateAllCharacteristicsForCurrentPattern();
}

void GoToPattern(int patternIndex)
{
	currentWavePlayerIndex = patternIndex;
	sharedCurrentIndexState = 0;
	Serial.println("GoToPattern" + String(currentWavePlayerIndex));
	SwitchWavePlayerIndex(currentWavePlayerIndex);
	UpdateAllCharacteristicsForCurrentPattern();
}

void UpdatePattern(Button::Event buttonEvent)
{
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
			for (int i = 0; i < NUM_LEDS; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(300, 1);
			break;
		}
		case PatternType::RING_PATTERN:
		{
			DrawRing(sharedCurrentIndexState % 4, leds, CRGB::DarkRed);
			IncrementSharedCurrentIndexState(160, 1);
			break;
		}
		case PatternType::COLUMN_PATTERN:
		{
			sharedIndices = GetIndicesForColumn(sharedCurrentIndexState % 8);
			DrawColumnOrRow(leds, sharedIndices, CRGB::DarkBlue);
			IncrementSharedCurrentIndexState(160, 1);
			break;
		}
		case PatternType::ROW_PATTERN:
		{
			sharedIndices = GetIndicesForRow(sharedCurrentIndexState % 8);
			DrawColumnOrRow(leds, sharedIndices, CRGB::DarkGreen);
			IncrementSharedCurrentIndexState(160, 1);
			break;
		}
		case PatternType::DIAGONAL_PATTERN:
		{
			sharedIndices = GetIndicesForDiagonal(sharedCurrentIndexState % 4);
			DrawColumnOrRow(leds, sharedIndices, CRGB::SlateGray);
			IncrementSharedCurrentIndexState(160, 1);
			break;
		}
		case PatternType::WAVE_PLAYER_PATTERN:
		{
			wavePlayer.update(wavePlayerSpeeds[currentWavePlayerIndex] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[currentWavePlayerIndex], 1);
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
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(300, 1);
			break;
		}
		default:
		{
			DrawError(CRGB::Red);
			break;
		}
	}

	for (auto &player : firedPatternPlayers)
	{
		player.update();
	}

	for (int i = 0; i < NUM_LEDS; ++i)
	{
		leds[i].r = LightArr[i].r;
		leds[i].g = LightArr[i].g;
		leds[i].b = LightArr[i].b;
	}
}

void UpdateCurrentPatternColors(Light newHighLt, Light newLowLt)
{
	const auto currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	switch (currentPattern)
	{
		case PatternType::WAVE_PLAYER_PATTERN:
			wavePlayer.hiLt = newHighLt;
			wavePlayer.loLt = newLowLt;
			wavePlayer.init(LightArr[0], wavePlayer.rows, wavePlayer.cols, newHighLt, newLowLt);
			break;
		case PatternType::DADS_PATTERN_PLAYER:
			break;
	}
	UpdateAllCharacteristicsForCurrentPattern();
}

WavePlayer *GetCurrentWavePlayer()
{
	const auto currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	switch (currentPattern)
	{
		case PatternType::WAVE_PLAYER_PATTERN: return &wavePlayer;
		default: return nullptr;
	}
}

std::pair<Light, Light> GetCurrentPatternColors()
{
	const auto currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	switch (currentPattern)
	{
		case PatternType::WAVE_PLAYER_PATTERN:
			return std::make_pair(wavePlayer.hiLt, wavePlayer.loLt);
		case PatternType::DADS_PATTERN_PLAYER:
		case PatternType::DATA_PATTERN:
		case PatternType::RING_PATTERN:
		case PatternType::COLUMN_PATTERN:
		case PatternType::ROW_PATTERN:
		case PatternType::DIAGONAL_PATTERN:
		default:
			return std::make_pair(Light(0, 0, 0), Light(0, 0, 0));
	}
}

void FirePatternFromBLE(int idx, Light on, Light off)
{
	const auto numPatterns = lp2Data.size();
	if (idx < 0 || idx >= numPatterns)
	{
		Serial.println("Invalid pattern index - must be 0-" + String(numPatterns - 1));
		return;
	}
	Serial.println("Trying to fire pattern " + String(idx));
	const auto playerIdx = findAvailablePatternPlayer();
	if (playerIdx == -1)
	{
		Serial.println("No available pattern player found");
		return;
	}
	Serial.println("Firing pattern " + String(idx) + " on player " + String(playerIdx));
	firedPatternPlayers[playerIdx].setToPlaySinglePattern(true);
	firedPatternPlayers[playerIdx].drawOffLt = false;
	firedPatternPlayers[playerIdx].onLt = on;
	firedPatternPlayers[playerIdx].offLt = off;
	firedPatternPlayers[playerIdx].firePattern(idx);
}

void IncrementSharedCurrentIndexState(unsigned int limit, unsigned int count)
{
	sharedCurrentIndexState += count;
	if (!ONLY_PUSHBUTTON_PATTERN_CHANGE && sharedCurrentIndexState >= limit)
	{
		GoToNextPattern();
	}
}

unsigned int findAvailablePatternPlayer()
{
	for (unsigned int i = 0; i < firedPatternPlayers.size(); ++i)
	{
		if (!firedPatternPlayers[i].isPlayingSinglePattern())
		{
			return i;
		}
	}
	return -1;
}
