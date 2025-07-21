#include "PatternManager.h"
#include <FastLED.h>
#include "Globals.h"
#include "../lights/Light.h"
#include "DeviceState.h"
#include <array>
#include <math.h>
#include "BLEManager.h"
#include "GlobalState.h"
#include "Utils.hpp"
#include "Behaviors/Ring.hpp"
#include "Behaviors/ColumnsRows.hpp"
#include "Behaviors/Diagonals.hpp"
#include "freertos/LogManager.h"
#include "config/JsonSettings.h"

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

// Forward declarations for BLE manager access
extern BLEManager bleManager;

// --- Pattern Logic Isolation ---
extern JsonSettings settings;
extern SDCardController *g_sdCardController;
DynamicJsonDocument patternsDoc(8196);

WavePlayerConfig jsonWavePlayerConfigs[10];

// Try loading a couple from /data/patterns.json
bool LoadPatternsFromJson() {
	if (g_sdCardController->isAvailable()) {
		LOG_DEBUG("Loading patterns from /data/patterns.json");

		String patternsJson = g_sdCardController->readFile("/data/patterns.json");
		DeserializationError error = deserializeJson(patternsDoc, patternsJson);
		if (error) {
			LOG_ERRORF("Failed to deserialize patterns JSON: %s", error.c_str());
			return false;
		}
		LOG_DEBUG("Patterns loaded successfully");
		return true;
	}
	return false;
}

void LoadWavePlayerConfigsFromJsonDocument() {
	LOG_DEBUG("Loading wave player configs from JSON document");
	if (patternsDoc.isNull()) {
		LOG_ERROR("Patterns document is null");
		return;
	}

	JsonArray wavePlayerConfigsArray = patternsDoc["wavePlayerConfigs"];
	if (wavePlayerConfigsArray.isNull()) {
		LOG_ERROR("Wave player configs array is null");
		return;
	}

	for (int i = 0; i < wavePlayerConfigsArray.size(); i++) {
		LOG_DEBUGF("Loading wave player config %d", i);
		const JsonObject &config = wavePlayerConfigsArray[i];
		WavePlayerConfig &wpConfig = jsonWavePlayerConfigs[i];
		wpConfig.name = config["name"].as<String>();
		wpConfig.rows = config["rows"].as<int>();
		wpConfig.cols = config["cols"].as<int>();
		wpConfig.onLight = Light(config["onLight"]["r"].as<int>(), config["onLight"]["g"].as<int>(), config["onLight"]["b"].as<int>());
		wpConfig.offLight = Light(config["offLight"]["r"].as<int>(), config["offLight"]["g"].as<int>(), config["offLight"]["b"].as<int>());
		wpConfig.AmpRt = config["AmpRt"].as<float>();
		wpConfig.wvLenLt = config["wvLenLt"].as<float>();
		wpConfig.wvLenRt = config["wvLenRt"].as<float>();
		wpConfig.wvSpdLt = config["wvSpdLt"].as<float>();
		wpConfig.wvSpdRt = config["wvSpdRt"].as<float>();
		wpConfig.rightTrigFuncIndex = config["rightTrigFuncIndex"].as<int>();
		wpConfig.leftTrigFuncIndex = config["leftTrigFuncIndex"].as<int>();
		wpConfig.useRightCoefficients = config["useRightCoefficients"].as<bool>();
		wpConfig.useLeftCoefficients = config["useLeftCoefficients"].as<bool>();
		wpConfig.nTermsRt = config["nTermsRt"].as<int>();
		wpConfig.nTermsLt = config["nTermsLt"].as<int>();
		for (int j = 0; j < 3; j++) {
			wpConfig.C_Rt[j] = config["C_Rt"][j].as<float>();
		}
		for (int j = 0; j < 3; j++) {
			wpConfig.C_Lt[j] = config["C_Lt"][j].as<float>();
		}
		wpConfig.setCoefficients(wpConfig.C_Rt, wpConfig.C_Lt);
		LOG_DEBUGF("Loaded wave player config %d: %s, %d, %d", i, wpConfig.name.c_str(), wpConfig.nTermsRt, wpConfig.nTermsLt);
		if (wpConfig.useRightCoefficients) {
			LOG_DEBUGF("C_Rt: %f, %f, %f", wpConfig.C_Rt[0], wpConfig.C_Rt[1], wpConfig.C_Rt[2]);
		}
		if (wpConfig.useLeftCoefficients) {
			LOG_DEBUGF("C_Lt: %f, %f, %f", wpConfig.C_Lt[0], wpConfig.C_Lt[1], wpConfig.C_Lt[2]);
		}
	}
	
}

WavePlayer testWavePlayer;
float C_Rt[3] = { 3,2,1 };


void Pattern_Setup() {

	// if (LoadPatternsFromJson()) {
	// 	LoadWavePlayerConfigsFromJsonDocument();
	// }

	/*
			int rows = 16, cols = 16;
		Light onLight = Light(7, 0, 255);
		Light offLight = Light(142, 90, 142);
		wp.rows = rows;
		wp.cols = cols;
		wp.onLight = onLight;
		wp.offLight = offLight;
		wp.AmpRt = 0.5;
		wp.wvLenLt = 87.652;
		wp.wvLenRt = 111.304344;
		wp.wvSpdLt = 23.652174;
		wp.wvSpdRt = 43.826;
		wp.rightTrigFuncIndex = 0;
		wp.leftTrigFuncIndex = 0;
		wp.nTermsRt = 3;
		wp.nTermsLt = 0;
		wp.useRightCoefficients = true;
		wp.useLeftCoefficients = false;*/
	testWavePlayer.init(LightArr[0], 16, 16, Light(7, 0, 255), Light(142, 90, 142));
	testWavePlayer.setWaveData(0.5, 87.652, 23.652174, 111.304344, 43.826);
	testWavePlayer.setSeriesCoeffs_Unsafe(C_Rt, 3, nullptr, 0);
	testWavePlayer.update(0.001f);

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
	lp2Data[0].init(1, 1, 2);
	lp2Data[0].init(2, 1, 2);
	lp2Data[1].init(3, 1, 10);
	lp2Data[2].init(4, 1, 10);
	lp2Data[3].init(5, 1, 8);
	lp2Data[4].init(6, 1, 10);
	lp2Data[5].init(7, 2, 10);
	lp2Data[6].init(10, 2, 8);
	lp2Data[7].init(11, 2, 8);
	lp2Data[8].init(12, 2, 8);
	lp2Data[9].init(13, 2, 8);
	lp2Data[10].init(14, 2, 10);
	lp2Data[11].init(15, 2, 10);
	lp2Data[12].init(16, 2, 10);
	lp2Data[13].init(31, 2, 10);
	lp2Data[14].init(32, 2, 10);
	lp2Data[15].init(33, 2, 10);
	lp2Data[16].init(34, 2, 8);
	lp2Data[17].init(80, 2, 8);
	lp2Data[16].init(40, 1, 8);
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
	UpdateBrightnessPulse();
    FastLED.show();
}

void Pattern_HandleBLE(const String& characteristic, const String& value) {
    if (characteristic == "patternIndex") {
        GoToPattern(value.toInt());
    } else if (characteristic == "highColor") {
        // Parse and update color
    } else if (characteristic == "firePattern") {
        Serial.println("Firing pattern " + value);
    }
    // ...etc...
}

void Pattern_FireSingle(int idx, Light on, Light off) {
    FirePatternFromBLE(idx, on, off);
}
// --- End Pattern Logic Isolation ---

void SwitchWavePlayerIndex(int index)
{
	auto& config = wavePlayerConfigs[index];
	// auto& config = jsonWavePlayerConfigs[index];
	wavePlayer.nTermsLt = wavePlayer.nTermsRt = 0;
	wavePlayer.C_Lt = wavePlayer.C_Rt = nullptr;
	wavePlayer.init(LightArr[0], config.rows, config.cols, config.onLight, config.offLight);
	wavePlayer.setWaveData(config.AmpRt, config.wvLenLt, config.wvSpdLt, config.wvLenRt, config.wvSpdRt);
	if (config.useLeftCoefficients || config.useRightCoefficients) {
		LOG_DEBUGF("WAVEPLAYER: Setting series coefficients: %f, %f, %f", config.C_Rt[0], config.C_Rt[1], config.C_Rt[2]);
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
			testWavePlayer.update(wavePlayerSpeeds[currentWavePlayerIndex] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				// leds[i].r = LightArr[i].r;
				// leds[i].g = LightArr[i].g;
				// leds[i].b = LightArr[i].b;
				leds[i].r = testWavePlayer.pLt0[i].r;
				leds[i].g = testWavePlayer.pLt0[i].g;
				leds[i].b = testWavePlayer.pLt0[i].b;
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

// ===== Functions moved from main.cpp =====

void UpdateAllCharacteristicsForCurrentPattern()
{
	// Update pattern index characteristic
	bleManager.getPatternIndexCharacteristic().writeValue(String(currentWavePlayerIndex));

	// Update color characteristics based on current pattern
	const auto currentColors = GetCurrentPatternColors();
	String highColorStr = String(currentColors.first.r) + "," + String(currentColors.first.g) + "," + String(currentColors.first.b);
	String lowColorStr = String(currentColors.second.r) + "," + String(currentColors.second.g) + "," + String(currentColors.second.b);

	bleManager.getHighColorCharacteristic().writeValue(highColorStr);
	bleManager.getLowColorCharacteristic().writeValue(lowColorStr);

	// Update brightness characteristic
	bleManager.updateBrightness();

	// Update series coefficients if applicable
	const auto currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	if (currentPattern == PatternType::WAVE_PLAYER_PATTERN)
	{
		// Get the appropriate wave player for the current pattern
		WavePlayer *currentWavePlayer = GetCurrentWavePlayer();

		if (currentWavePlayer)
		{
			// Format series coefficients as strings
			String leftCoeffsStr = "0.0,0.0,0.0";
			String rightCoeffsStr = "0.0,0.0,0.0";

			if (currentWavePlayer->C_Lt && currentWavePlayer->nTermsLt > 0)
			{
				leftCoeffsStr = String(currentWavePlayer->C_Lt[0], 2) + "," +
					String(currentWavePlayer->C_Lt[1], 2) + "," +
					String(currentWavePlayer->C_Lt[2], 2);
			}

			if (currentWavePlayer->C_Rt && currentWavePlayer->nTermsRt > 0)
			{
				rightCoeffsStr = String(currentWavePlayer->C_Rt[0], 2) + "," +
					String(currentWavePlayer->C_Rt[1], 2) + "," +
					String(currentWavePlayer->C_Rt[2], 2);
			}

			bleManager.getLeftSeriesCoefficientsCharacteristic().writeValue(leftCoeffsStr);
			bleManager.getRightSeriesCoefficientsCharacteristic().writeValue(rightCoeffsStr);
		}
	}
}

void UpdateColorFromCharacteristic(BLEStringCharacteristic &characteristic, Light &color, bool isHighColor)
{
	const auto value = characteristic.value();
	Serial.println("Color characteristic written" + String(value));
	// Cast from int,int,int to strings, but integers might not all be the same length
	// So parse from comma to comma

	// Split the string into r,g,b
	String r = value.substring(0, value.indexOf(','));
	String g = value.substring(value.indexOf(',') + 1, value.lastIndexOf(','));
	String b = value.substring(value.lastIndexOf(',') + 1);


	Serial.println("R: " + String(r));
	Serial.println("G: " + String(g));
	Serial.println("B: " + String(b));

	int rInt = r.toInt();
	int gInt = g.toInt();
	int bInt = b.toInt();
	Serial.println("Setting color to: " + String(rInt) + "," + String(gInt) + "," + String(bInt));
	Light newColor = Light(rInt, gInt, bInt);
	const auto currentPatternColors = GetCurrentPatternColors();
	if (isHighColor)
	{
		UpdateCurrentPatternColors(newColor, currentPatternColors.second);
	}
	else
	{
		UpdateCurrentPatternColors(currentPatternColors.first, newColor);
	}
}

void UpdateSeriesCoefficientsFromCharacteristic(BLEStringCharacteristic &characteristic, WavePlayer &wp)
{
	const auto value = characteristic.value();
	Serial.println("Series coefficients characteristic written" + String(value));

	float leftCoeffsArray[3];
	float rightCoeffsArray[3];

	if (wp.nTermsLt > 0)
	{
		// Parse from string like 0.95,0.3,1.2
		String coeffs = value.substring(0, value.indexOf(','));
		String coeffs2 = value.substring(value.indexOf(',') + 1, value.lastIndexOf(','));
		String coeffs3 = value.substring(value.lastIndexOf(',') + 1);
		leftCoeffsArray[0] = coeffs.toFloat();
		leftCoeffsArray[1] = coeffs2.toFloat();
		leftCoeffsArray[2] = coeffs3.toFloat();
	}

	if (wp.nTermsRt > 0)
	{
		// Parse from string like 0.95,0.3,1.2
		String coeffs = value.substring(0, value.indexOf(','));
		String coeffs2 = value.substring(value.indexOf(',') + 1, value.lastIndexOf(','));
		String coeffs3 = value.substring(value.lastIndexOf(',') + 1);
		rightCoeffsArray[0] = coeffs.toFloat();
		rightCoeffsArray[1] = coeffs2.toFloat();
		rightCoeffsArray[2] = coeffs3.toFloat();
	}

	wp.setSeriesCoeffs_Unsafe(leftCoeffsArray, 3, rightCoeffsArray, 3);

	// Update characteristics to reflect the new series coefficients
	UpdateAllCharacteristicsForCurrentPattern();
}

Light ParseColor(const String &colorStr)
{
	// Parse from string like (r,g,b)
	String r = colorStr.substring(0, colorStr.indexOf(','));
	String g = colorStr.substring(colorStr.indexOf(',') + 1, colorStr.lastIndexOf(','));
	String b = colorStr.substring(colorStr.lastIndexOf(',') + 1);
	int rInt = r.toInt();
	int gInt = g.toInt();
	int bInt = b.toInt();
	return Light(rInt, gInt, bInt);
}

// Parse a fire_pattern command like fire_pattern:<idx>-(args-args-args,etc)
// but here we only need to get the idx and the rest is handled by
// the callee (firePatternFromBLE handles dealing with the args and turning it into a
// param if needed, or on/off colors, etc)
void ParseFirePatternCommand(const String &command)
{
	// Find the dash separator
	int dashIndex = command.indexOf('-');
	if (dashIndex == -1)
	{
		Serial.println("Invalid fire_pattern format - expected idx-on-off");
		return;
	}

	// Extract the index
	String idxStr = command.substring(0, dashIndex);
	int idx = idxStr.toInt();

	// Extract the on and off colors
	String onStr = command.substring(dashIndex + 1, command.indexOf('-', dashIndex + 1));
	String offStr = command.substring(command.indexOf('-', dashIndex + 1) + 1);

	// Parse the on and off colors
	Light on = ParseColor(onStr);
	Light off = ParseColor(offStr);
	FirePatternFromBLE(idx, on, off);
}

void ParseAndExecuteCommand(const String &command)
{
	Serial.println("Parsing command: " + command);

	// Find the colon separator
	int colonIndex = command.indexOf(':');
	if (colonIndex == -1)
	{
		Serial.println("Invalid command format - missing colon");
		return;
	}

	// Extract command and arguments
	String cmd = command.substring(0, colonIndex);
	String args = command.substring(colonIndex + 1);

	cmd.trim();
	args.trim();

	Serial.println("Command: " + cmd);
	Serial.println("Args: " + args);

	if (cmd == "pulse_brightness")
	{
		// Parse arguments: target_brightness,duration_ms
		int commaIndex = args.indexOf(',');
		if (commaIndex == -1)
		{
			Serial.println("Invalid pulse_brightness format - expected target,duration");
			return;
		}

		String targetStr = args.substring(0, commaIndex);
		String durationStr = args.substring(commaIndex + 1);

		int targetBrightness = targetStr.toInt();
		unsigned long duration = durationStr.toInt();

		if (targetBrightness < 0 || targetBrightness > 255)
		{
			Serial.println("Invalid target brightness - must be 0-255");
			return;
		}

		if (duration <= 0)
		{
			Serial.println("Invalid duration - must be positive");
			return;
		}

		StartBrightnessPulse(targetBrightness, duration);
		Serial.println("Started brightness pulse to " + String(targetBrightness) + " over " + String(duration) + "ms");
	}
	else if (cmd == "fade_brightness")
	{
		// Parse arguments: target_brightness,duration_ms
		int commaIndex = args.indexOf(',');
		if (commaIndex == -1)
		{
			Serial.println("Invalid fade_brightness format - expected target,duration");
			return;
		}

		String targetStr = args.substring(0, commaIndex);
		String durationStr = args.substring(commaIndex + 1);

		int targetBrightness = targetStr.toInt();
		unsigned long duration = durationStr.toInt();

		if (targetBrightness < 0 || targetBrightness > 255)
		{
			Serial.println("Invalid target brightness - must be 0-255");
			return;
		}

		if (duration <= 0)
		{
			Serial.println("Invalid duration - must be positive");
			return;
		}

		StartBrightnessFade(targetBrightness, duration);
		Serial.println("Started brightness fade to " + String(targetBrightness) + " over " + String(duration) + "ms");
	}
	else if (cmd == "fire_pattern")
	{
		// Command will look like fire_pattern:<idx>-<string-args>
		ParseFirePatternCommand(args);
	}
	else if (cmd == "ping") {
		// Echo back the timestamp
		Serial.println("Ping -- pong");
		bleManager.getCommandCharacteristic().writeValue("pong:" + args);
	}
	else
	{
		Serial.println("Unknown command: " + cmd);
	}
}

void StartBrightnessPulse(int targetBrightness, unsigned long duration)
{
	// Store current brightness before starting pulse
	extern DeviceState deviceState;
	previousBrightness = deviceState.brightness;
	pulseTargetBrightness = targetBrightness;
	pulseDuration = duration;
	pulseStartTime = millis();
	isPulsing = true;
	isFadeMode = false;  // This is a pulse, not a fade

	Serial.println("Starting brightness pulse from " + String(previousBrightness) + " to " + String(targetBrightness));
}

void StartBrightnessFade(int targetBrightness, unsigned long duration)
{
	// Store current brightness before starting fade
	extern DeviceState deviceState;
	previousBrightness = deviceState.brightness;
	pulseTargetBrightness = targetBrightness;
	pulseDuration = duration;
	pulseStartTime = millis();
	isPulsing = true;
	isFadeMode = true;  // This is a fade, not a pulse

	Serial.println("Starting brightness fade from " + String(previousBrightness) + " to " + String(targetBrightness));
}

void UpdateBrightnessPulse()
{
	if (!isPulsing)
	{
		return;
	}

	unsigned long currentTime = millis();
	unsigned long elapsed = currentTime - pulseStartTime;

	if (elapsed >= pulseDuration)
	{
		// Transition complete - set to final brightness
		extern DeviceState deviceState;
		extern BLEManager bleManager;
		
		if (isFadeMode) {
			// For fade, stay at target brightness
			deviceState.brightness = pulseTargetBrightness;
			FastLED.setBrightness(deviceState.brightness);
			bleManager.updateBrightness();
			Serial.println("Brightness fade complete - now at " + String(pulseTargetBrightness));
		} else {
			// For pulse, return to previous brightness
			deviceState.brightness = previousBrightness;
			FastLED.setBrightness(deviceState.brightness);
			bleManager.updateBrightness();
			Serial.println("Brightness pulse complete - returned to " + String(previousBrightness));
		}
		isPulsing = false;
		return;
	}

	// Calculate current brightness using smooth interpolation
	float progress = (float) elapsed / (float) pulseDuration;

	float smoothProgress;
	if (isFadeMode) {
		// Linear interpolation for fade (simple and smooth)
		smoothProgress = progress;
	} else {
		// Use smooth sine wave interpolation for natural pulsing effect
		// This creates a full cycle: 0 -> 1 -> 0 over the duration
		smoothProgress = (sin(progress * 2 * PI - PI / 2) + 1) / 2; // Full cycle: 0 to 1 to 0
	}

	extern DeviceState deviceState;
	extern BLEManager bleManager;
	int currentBrightness = previousBrightness + (int) ((pulseTargetBrightness - previousBrightness) * smoothProgress);

	// Apply the calculated brightness
	deviceState.brightness = currentBrightness;
	FastLED.setBrightness(deviceState.brightness);
	bleManager.updateBrightness();
}

void UpdateBrightnessInt(int value)
{
	extern DeviceState deviceState;
	extern BLEManager bleManager;
	deviceState.brightness = value;
	FastLED.setBrightness(deviceState.brightness);
	bleManager.updateBrightness();
}

void UpdateBrightness(float value)
{
	extern DeviceState deviceState;
	deviceState.brightness = value * 255;
	FastLED.setBrightness(deviceState.brightness);
}


void ApplyFromUserPreferences(DeviceState &state) {
	speedMultiplier = state.speedMultiplier;
	Serial.println("Applying from user preferences: speedMultiplier = " + String(speedMultiplier));
	UpdateBrightnessInt(state.brightness);
	Serial.println("Applying from user preferences: brightness = " + String(state.brightness));
	GoToPattern(state.patternIndex);
	Serial.println("Applying from user preferences: patternIndex = " + String(state.patternIndex));
	// UpdateCurrentPatternColors(state.highColor, state.lowColor);
	// Serial.println("Applying from user preferences: highColor = " + String(state.highColor.r) + "," + String(state.highColor.g) + "," + String(state.highColor.b));
	// Serial.println("Applying from user preferences: lowColor = " + String(state.lowColor.r) + "," + String(state.lowColor.g) + "," + String(state.lowColor.b));
}

void SaveUserPreferences(const DeviceState& state) {
	Serial.println("Saving user preferences");
	prefsManager.save(state);
}