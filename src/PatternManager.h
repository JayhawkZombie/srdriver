#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <array>
#include "Light.h"
#include "DeviceState.h"
#include "hal/Button.hpp"
#include "LightPlayer2.h"
#include "WavePlayer.h"
#include "DataPlayer.h"

// Pattern types
enum class PatternType {
    DADS_PATTERN_PLAYER,
    RING_PATTERN,
    COLUMN_PATTERN,
    ROW_PATTERN,
    DIAGONAL_PATTERN,
    WAVE_PLAYER_PATTERN,
    DATA_PATTERN,
};

extern std::array<PatternType, 20> patternOrder;
extern size_t patternOrderSize;
extern int currentWavePlayerIndex;
extern int currentPatternIndex;
extern std::array<patternData, 40> lp2Data;
extern std::array<LightPlayer2, 40> firedPatternPlayers;
extern WavePlayerConfig wavePlayerConfigs[10];
extern Light LightArr[NUM_LEDS];
extern Button pushButton;
extern Button pushButtonSecondary;
extern float wavePlayerSpeeds[];
extern int wavePlayerLengths[];
extern DataPlayer dp;
constexpr int numWavePlayerConfigs = 10;

// Externs for pattern logic
extern void initWaveData(WavePlayerConfig &wp);
extern void initWaveData2(WavePlayerConfig &wp);
extern void initWaveData3(WavePlayerConfig &wp);
extern void initWaveData4(WavePlayerConfig &wp);
extern void initWaveData5(WavePlayerConfig &wp);
extern void initWaveData6(WavePlayerConfig &wp);
extern void initWaveData7(WavePlayerConfig &wp);
extern void initWaveData8(WavePlayerConfig &wp);
extern void initWaveData9(WavePlayerConfig &wp);
extern void initWaveData10(WavePlayerConfig &wp);
extern void SwitchWavePlayerIndex(int index);
extern void UpdatePattern(Button::Event buttonEvent);
extern void GoToPattern(int patternIndex);
extern void FirePatternFromBLE(int idx, Light on, Light off);

extern CRGB leds[];
extern float speedMultiplier;
extern int sharedCurrentIndexState;
extern fl::FixedVector<int, LEDS_MATRIX_Y> sharedIndices;
extern void DrawRing(int, CRGB*, const CRGB&);
extern void DrawColumnOrRow(CRGB*, const fl::FixedVector<int, LEDS_MATRIX_Y>&, const CRGB&);
extern fl::FixedVector<int, LEDS_MATRIX_Y> GetIndicesForColumn(int);
extern fl::FixedVector<int, LEDS_MATRIX_Y> GetIndicesForRow(int);
extern fl::FixedVector<int, LEDS_MATRIX_Y> GetIndicesForDiagonal(int);
extern void DrawError(const CRGB&);
extern void UpdateAllCharacteristicsForCurrentPattern();
extern void IncrementSharedCurrentIndexState(unsigned int limit, unsigned int count);

extern void Pattern_Setup();
extern void Pattern_Loop();
extern void Pattern_HandleBLE(const String& characteristic, const String& value);
extern void Pattern_FireSingle(int idx, Light on, Light off);
extern void GoToPattern(int patternIndex);
extern void GoToNextPattern();
extern void SwitchWavePlayerIndex(int index);
extern void UpdatePattern(Button::Event buttonEvent);
extern void UpdateCurrentPatternColors(Light newHighLt, Light newLowLt);
extern std::pair<Light, Light> GetCurrentPatternColors();
extern WavePlayer* GetCurrentWavePlayer();
extern void FirePatternFromBLE(int idx, Light on, Light off);

unsigned int findAvailablePatternPlayer();