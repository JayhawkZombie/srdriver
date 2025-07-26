#include "PatternManager.h"
#include <FastLED.h>
#include "Globals.h"
#include "../lights/Light.h"
#include "DeviceState.h"
#include <array>
#include <math.h>
#include "hal/ble/BLEManager.h"
#include "GlobalState.h"
#include "Utils.hpp"
#include "freertos/LogManager.h"
#include "config/JsonSettings.h"

// Add externs for all globals and helpers used by pattern logic
unsigned int findAvailablePatternPlayer();
void SetupWavePlayerCoefficients(const WavePlayerConfig &config, float *C_Rt, float *C_Lt, float **rightCoeffs, float **leftCoeffs, int *nTermsRt, int *nTermsLt);

// Pattern-related global definitions
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

// Forward declarations for BLE manager access
extern BLEManager bleManager;

// --- Pattern Logic Isolation ---
extern JsonSettings settings;
extern SDCardController *g_sdCardController;
DynamicJsonDocument patternsDoc(8196);

WavePlayerConfig jsonWavePlayerConfigs[10];

// Try loading a couple from /data/patterns.json
bool LoadPatternsFromJson()
{
	if (g_sdCardController->isAvailable())
	{
		LOG_DEBUG("Loading patterns from /data/patterns.json");

		String patternsJson = g_sdCardController->readFile("/data/patterns.json");
		DeserializationError error = deserializeJson(patternsDoc, patternsJson);
		if (error)
		{
			LOG_ERRORF("Failed to deserialize patterns JSON: %s", error.c_str());
			return false;
		}
		LOG_DEBUG("Patterns loaded successfully");
		return true;
	}
	return false;
}

void LoadWavePlayerConfigsFromJsonDocument()
{
	LOG_DEBUG("Loading wave player configs from JSON document");
	if (patternsDoc.isNull())
	{
		LOG_ERROR("Patterns document is null");
		return;
	}

	JsonArray wavePlayerConfigsArray = patternsDoc["wavePlayerConfigs"];
	if (wavePlayerConfigsArray.isNull())
	{
		LOG_ERROR("Wave player configs array is null");
		return;
	}

	for (int i = 0; i < wavePlayerConfigsArray.size(); i++)
	{
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
		const auto lCoeff = config["C_Lt"];
		const auto rCoeff = config["C_Rt"];

		// if (wpConfig.useRightCoefficients) {
		for (int j = 0; j < 3; j++)
		{
			wpConfig.C_Rt[j] = config["C_Rt"][j].as<float>();
		}
		// }
		// if (wpConfig.useLeftCoefficients) {
		for (int j = 0; j < 3; j++)
		{
			wpConfig.C_Lt[j] = config["C_Lt"][j].as<float>();
		}
		// }
		wpConfig.setCoefficients(wpConfig.C_Rt, wpConfig.C_Lt);
		LOG_DEBUGF("Loaded wave player config %d: %s, %d, %d", i, wpConfig.name.c_str(), wpConfig.nTermsRt, wpConfig.nTermsLt);
		if (wpConfig.useRightCoefficients)
		{
			LOG_DEBUGF("C_Rt: %f, %f, %f", wpConfig.C_Rt[0], wpConfig.C_Rt[1], wpConfig.C_Rt[2]);
		}
		if (wpConfig.useLeftCoefficients)
		{
			LOG_DEBUGF("C_Lt: %f, %f, %f", wpConfig.C_Lt[0], wpConfig.C_Lt[1], wpConfig.C_Lt[2]);
		}
	}
}

WavePlayer testWavePlayer;
float C_Rt[3] = { 3,2,1 };
float C_Lt[3] = { 3,2,1 };


void Pattern_Setup()
{

	if (LoadPatternsFromJson())
	{
		LoadWavePlayerConfigsFromJsonDocument();
	}

	auto &testConfig = jsonWavePlayerConfigs[1];
	LOG_DEBUGF("Using config index 1: %s", testConfig.name.c_str());
	LOG_DEBUGF("Config 1 - nTermsRt: %d, nTermsLt: %d", testConfig.nTermsRt, testConfig.nTermsLt);
	LOG_DEBUGF("Config 1 - C_Rt: %f, %f, %f", testConfig.C_Rt[0], testConfig.C_Rt[1], testConfig.C_Rt[2]);
	LOG_DEBUGF("Config 1 - C_Lt: %f, %f, %f", testConfig.C_Lt[0], testConfig.C_Lt[1], testConfig.C_Lt[2]);

	testWavePlayer.init(LightArr[0], testConfig.rows, testConfig.cols, testConfig.onLight, testConfig.offLight);
	testWavePlayer.setWaveData(testConfig.AmpRt, testConfig.wvLenLt, testConfig.wvSpdLt, testConfig.wvLenRt, testConfig.wvSpdRt);
	testWavePlayer.setRightTrigFunc(testConfig.rightTrigFuncIndex);
	testWavePlayer.setLeftTrigFunc(testConfig.leftTrigFuncIndex);

	// Use helper function to set up coefficients properly
	float *rightCoeffs, *leftCoeffs;
	int nTermsRt, nTermsLt;
	SetupWavePlayerCoefficients(testConfig, C_Rt, C_Lt, &rightCoeffs, &leftCoeffs, &nTermsRt, &nTermsLt);

	testWavePlayer.setSeriesCoeffs_Unsafe(rightCoeffs, nTermsRt, leftCoeffs, nTermsLt);
	testWavePlayer.update(0.001f);

	// initWaveData(wavePlayerConfigs[0]);
	// initWaveData2(wavePlayerConfigs[1]);
	// initWaveData3(wavePlayerConfigs[2]);
	// initWaveData4(wavePlayerConfigs[3]);
	// initWaveData5(wavePlayerConfigs[4]);
	// initWaveData6(wavePlayerConfigs[5]);
	// initWaveData7(wavePlayerConfigs[6]);
	// initWaveData8(wavePlayerConfigs[7]);
	// initWaveData9(wavePlayerConfigs[8]);
	// initWaveData10(wavePlayerConfigs[9]);
	// SwitchWavePlayerIndex(0);
	// To avoid flashes when loading user settings
	UpdateBrightnessInt(0);
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
	for (auto &player : firedPatternPlayers)
	{
		player.onLt = Light(255, 255, 255);
		player.offLt = Light(0, 0, 0);
		player.init(LightArr[0], 1, 120, lp2Data[0], 18);
		player.drawOffLt = false;
		player.setToPlaySinglePattern(true);
		player.update();
	}
}

void Pattern_Loop()
{
	UpdatePattern(pushButton.getEvent());
	UpdateBrightnessPulse();
	FastLED.show();
}

// --- End Pattern Logic Isolation ---

// Helper function to properly set up wave player coefficients
void SetupWavePlayerCoefficients(const WavePlayerConfig &config, float *C_Rt, float *C_Lt,
	float **rightCoeffs, float **leftCoeffs,
	int *nTermsRt, int *nTermsLt)
{
	// Copy right coefficients
	if (config.useRightCoefficients && config.nTermsRt > 0)
	{
		for (int i = 0; i < 3; ++i)
		{
			C_Rt[i] = config.C_Rt[i];
		}
		*rightCoeffs = C_Rt;
		*nTermsRt = config.nTermsRt;
	}
	else
	{
		*rightCoeffs = nullptr;
		*nTermsRt = 0;
	}

	// Handle left coefficients
	if (config.useLeftCoefficients && config.nTermsLt > 0)
	{
		for (int i = 0; i < 3; ++i)
		{
			C_Lt[i] = config.C_Lt[i];
		}
		*leftCoeffs = C_Lt;
		*nTermsLt = config.nTermsLt;
	}
	else if (config.wvLenLt > 0 && config.wvSpdLt > 0)
	{
		// Use basic trig function for left wave (no coefficients)
		*leftCoeffs = nullptr;
		*nTermsLt = 1;
	}
	else
	{
		// No left wave
		*leftCoeffs = nullptr;
		*nTermsLt = 0;
	}

	LOG_DEBUGF("Setup coefficients - right: %s, left: %s",
		*rightCoeffs ? "coefficients" : "basic trig",
		*leftCoeffs ? "coefficients" : (*nTermsLt > 0 ? "basic trig" : "none"));
}


void SwitchWavePlayerIndex(int index)
{
	// auto &config = wavePlayerConfigs[index];
	auto &config = jsonWavePlayerConfigs[index];
	LOG_DEBUGF("Switching to config %d: %s", index, config.name.c_str());
	LOG_DEBUGF("Config %d - nTermsRt: %d, nTermsLt: %d", index, config.nTermsRt, config.nTermsLt);
	LOG_DEBUGF("Config %d - C_Rt: %f, %f, %f", index, config.C_Rt[0], config.C_Rt[1], config.C_Rt[2]);
	LOG_DEBUGF("Config %d - C_Lt: %f, %f, %f", index, config.C_Lt[0], config.C_Lt[1], config.C_Lt[2]);

	testWavePlayer.nTermsLt = config.nTermsLt;
	testWavePlayer.nTermsRt = config.nTermsRt;

	testWavePlayer.init(LightArr[0], config.rows, config.cols, config.onLight, config.offLight);
	testWavePlayer.setWaveData(config.AmpRt, config.wvLenLt, config.wvSpdLt, config.wvLenRt, config.wvSpdRt);
	testWavePlayer.setRightTrigFunc(config.rightTrigFuncIndex);
	testWavePlayer.setLeftTrigFunc(config.leftTrigFuncIndex);

	// Use helper function to set up coefficients properly
	float *rightCoeffs, *leftCoeffs;
	int nTermsRt, nTermsLt;
	SetupWavePlayerCoefficients(config, C_Rt, C_Lt, &rightCoeffs, &leftCoeffs, &nTermsRt, &nTermsLt);

	testWavePlayer.setSeriesCoeffs_Unsafe(rightCoeffs, nTermsRt, leftCoeffs, nTermsLt);
}


void GoToNextPattern()
{
	currentWavePlayerIndex++;
	if (currentWavePlayerIndex >= numWavePlayerConfigs)
	{
		currentWavePlayerIndex = 0;
	}
	SwitchWavePlayerIndex(currentWavePlayerIndex);
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


	testWavePlayer.update(wavePlayerSpeeds[currentWavePlayerIndex] * speedMultiplier);
	IncrementSharedCurrentIndexState(wavePlayerLengths[currentWavePlayerIndex], 1);

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
	WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
	currentWavePlayer->hiLt = newHighLt;
	currentWavePlayer->loLt = newLowLt;
	currentWavePlayer->init(LightArr[0], currentWavePlayer->rows, currentWavePlayer->cols, newHighLt, newLowLt);
	UpdateAllCharacteristicsForCurrentPattern();
}

WavePlayer *GetCurrentWavePlayer()
{
	return &testWavePlayer;
}

std::pair<Light, Light> GetCurrentPatternColors()
{
	return std::make_pair(testWavePlayer.hiLt, testWavePlayer.loLt);
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
	else if (cmd == "ping")
	{
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

		if (isFadeMode)
		{
			// For fade, stay at target brightness
			deviceState.brightness = pulseTargetBrightness;
			FastLED.setBrightness(deviceState.brightness);
			bleManager.updateBrightness();
			Serial.println("Brightness fade complete - now at " + String(pulseTargetBrightness));
		}
		else
		{
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
	if (isFadeMode)
	{
		// Linear interpolation for fade (simple and smooth)
		smoothProgress = progress;
	}
	else
	{
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


void ApplyFromUserPreferences(DeviceState &state, bool skipBrightness)
{
	speedMultiplier = state.speedMultiplier;
	Serial.println("Applying from user preferences: speedMultiplier = " + String(speedMultiplier));
	if (!skipBrightness)
	{
		Serial.println("Applying from user preferences: brightness = " + String(state.brightness));
		UpdateBrightnessInt(state.brightness);
	}
	GoToPattern(state.patternIndex);
	Serial.println("Applying from user preferences: patternIndex = " + String(state.patternIndex));
	// UpdateCurrentPatternColors(state.highColor, state.lowColor);
	// Serial.println("Applying from user preferences: highColor = " + String(state.highColor.r) + "," + String(state.highColor.g) + "," + String(state.highColor.b));
	// Serial.println("Applying from user preferences: lowColor = " + String(state.lowColor.r) + "," + String(state.lowColor.g) + "," + String(state.lowColor.b));
}

void SaveUserPreferences(const DeviceState &state)
{
	Serial.println("Saving user preferences");
	prefsManager.save(state);
}