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
#include "hal/Button.hpp"
#include "hal/Potentiometer.hpp"
#include "die.hpp"
#include "WavePlayer.h"
#include <ArduinoBLE.h>

// Authentication system
#define AUTH_PIN "1234"  // Default PIN - change this to your preferred PIN
#define MAX_AUTHORIZED_DEVICES 5
String authorizedDevices[MAX_AUTHORIZED_DEVICES];
int numAuthorizedDevices = 0;
bool isAuthenticated = false;
bool pairingMode = false;
unsigned long pairingModeStartTime = 0;
const unsigned long PAIRING_TIMEOUT = 30000; // 30 seconds
bool controlServiceAdded = false; // Track if control service has been added

// Create separate services for security
BLEService authService("a1862b70-e0ce-4b1b-9734-d7629eb8d710");  // Auth service (always advertised)
BLEService controlService("b1862b70-e0ce-4b1b-9734-d7629eb8d711"); // Control service (only after auth)
int GlobalBrightness = 0;

// Brightness pulsing variables
bool isPulsing = false;
int pulseTargetBrightness = 0;
int previousBrightness = 0;
unsigned long pulseStartTime = 0;
unsigned long pulseDuration = 0;

// Auth characteristic (always available)
BLEStringCharacteristic authCharacteristic("a1b2c3d4-e5f6-7890-abcd-ef1234567890", BLERead | BLEWrite | BLENotify, 10);

// Control characteristics (only available after authentication)
BLEStringCharacteristic brightnessCharacteristic("4df3a1f9-2a42-43ee-ac96-f7db09abb4f0", BLERead | BLEWrite | BLENotify, 3);
BLEStringCharacteristic patternIndexCharacteristic("e95785e0-220e-4cd9-8839-7e92595e47b0", BLERead | BLEWrite | BLENotify, 4);
BLEStringCharacteristic highColorCharacteristic("932334a3-8544-4edc-ba49-15055eb1c877", BLERead | BLEWrite | BLENotify, 20);
BLEStringCharacteristic lowColorCharacteristic("8cdb8d7f-d2aa-4621-a91f-ca3f54731950", BLERead | BLEWrite | BLENotify, 20);
BLEStringCharacteristic leftSeriesCoefficientsCharacteristic("762ff1a5-8965-4d5c-b98e-4faf9b382267", BLERead | BLEWrite | BLENotify, 20);
BLEStringCharacteristic rightSeriesCoefficientsCharacteristic("386e0c80-fb59-4e8b-b5d7-6eca4d68ce33", BLERead | BLEWrite | BLENotify, 20);
BLEStringCharacteristic commandCharacteristic("c1862b70-e0ce-4b1b-9734-d7629eb8d712", BLERead | BLEWrite | BLENotify, 50);

// BLE Descriptors for human-readable names
BLEDescriptor brightnessDescriptor("2901", "Brightness Control");
BLEDescriptor patternIndexDescriptor("2901", "Pattern Index");
BLEDescriptor highColorDescriptor("2901", "High Color");
BLEDescriptor lowColorDescriptor("2901", "Low Color");
BLEDescriptor leftSeriesCoefficientsDescriptor("2901", "Left Series Coefficients");
BLEDescriptor rightSeriesCoefficientsDescriptor("2901", "Right Series Coefficients");
BLEDescriptor authDescriptor("2901", "Authentication");
BLEDescriptor commandDescriptor("2901", "Command Interface");

// Define a structure for the 0x2904 descriptor
struct BLE2904_Data {
	uint8_t m_format;
	int8_t m_exponent;
	uint16_t m_unit;
	uint8_t m_namespace;
	uint16_t m_description;
} __attribute__((packed));

// Create format descriptors for UTF-8 strings
BLE2904_Data stringFormat = {
	0x1A,      // m_format: UTF-8 String with null termination
	0,         // m_exponent: No exponent
	0x0000,    // m_unit: No unit
	0x01,      // m_namespace: Bluetooth SIG namespace
	0x0000     // m_description: No description
};

BLEDescriptor brightnessFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor patternIndexFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor highColorFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor lowColorFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor leftSeriesCoefficientsFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor rightSeriesCoefficientsFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor commandFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));

#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
Rgbw rgbw = Rgbw(
	kRGBWDefaultColorTemp,
	kRGBWExactColors,      // Mode
	W3                     // W-placement
);

typedef WS2812<LED_PIN, RGB> ControllerT;  // RGB mode must be RGB, no re-ordering allowed.
static RGBWEmulatedController<ControllerT, GRB> rgbwEmu(rgbw);  // ordering goes here.
#endif

// LightPlayer2 uses 
Light onLt(200, 0, 60);// these
Light offLt(60, 0, 200);// Lights

// Button and Potentiometer instances
Button pushButton(PUSHBUTTON_PIN);
Button pushButtonSecondary(PUSHBUTTON_PIN_SECONDARY);
Potentiometer brightnessPot(POTENTIOMETER_PIN_BRIGHTNESS);
Potentiometer speedPot(POTENTIOMETER_PIN_SPEED);
Potentiometer extraPot(POTENTIOMETER_PIN_EXTRA);

Light LightArr[NUM_LEDS];// storage for player
CRGB leds[NUM_LEDS];

LightPlayer2 LtPlay2; // Declare the LightPlayer2 instance

// storage for a 3 step pattern #100
uint8_t stateData[24];// enough for 24*8 = 192 = 3*64 state assignments

WavePlayer wavePlayer;
WavePlayer wavePlayer2;
WavePlayer wavePlayer3;
WavePlayer wavePlayer4;
WavePlayer wavePlayer5;
WavePlayer wavePlayer6;
WavePlayer wavePlayer7;
WavePlayer wavePlayer8;
WavePlayer wavePlayer9;
WavePlayer largeWavePlayer;
DataPlayer dataPlayer;

int currentWavePlayerIndex = 0;
constexpr int numWavePlayerConfigs = 9;

int wavePlayerLengths[numWavePlayerConfigs] = { 100, 100, 100, 300, 300, 300, 300, 300, 300 };
float wavePlayerSpeeds[numWavePlayerConfigs] = { 0.001f, 0.0035f, 0.003f, 0.001f, 0.001f, 0.0005f, 0.001f, 0.001f, 0.001f };

WavePlayerConfig wavePlayerConfigs[numWavePlayerConfigs] = {};

DataPlayer dp;

extern void initDataPlayer(DataPlayer &dp, uint8_t *data, uint16_t numData, Light *arr);
extern void initWaveData(WavePlayerConfig &wp);
extern void initWaveData2(WavePlayerConfig &wp);
extern void initWaveData3(WavePlayerConfig &wp);
extern void initWaveData4(WavePlayerConfig &wp);
extern void initWaveData5(WavePlayerConfig &wp);
extern void initWaveData6(WavePlayerConfig &wp);
extern void initWaveData7(WavePlayerConfig &wp);
extern void initWaveData8(WavePlayerConfig &wp);
extern void initWaveData9(WavePlayerConfig &wp);
extern void initLargeWaveData(WavePlayerConfig &wp);

void SwitchWavePlayerIndex(int index)
{
	const auto config = wavePlayerConfigs[index];
	if (config.useLeftCoefficients || config.useRightCoefficients) {
		wavePlayer.setSeriesCoeffs_Unsafe(config.C_Rt, config.nTermsRt, config.C_Lt, config.nTermsLt);
	}
	wavePlayer.init(LightArr[0], config.rows, config.cols, config.onLight, config.offLight);
	wavePlayer.setWaveData(config.AmpRt, config.wvLenLt, config.wvSpdLt, config.wvLenRt, config.wvSpdRt);
}

void EnterSettingsMode();
void MoveToNextSetting();
void ExitSettingsMode();
void UpdateLEDsForSettings(int potentiometerValue);
std::pair<Light, Light> GetCurrentPatternColors();
WavePlayer *GetCurrentWavePlayer();
void ParseAndExecuteCommand(const String &command);
void StartBrightnessPulse(int targetBrightness, unsigned long duration);
void UpdateBrightnessPulse();

enum class PatternType
{
	DADS_PATTERN_PLAYER,
	RING_PATTERN,
	COLUMN_PATTERN,
	ROW_PATTERN,
	DIAGONAL_PATTERN,
	WAVE_PLAYER_PATTERN,
	DATA_PATTERN,
};

fl::FixedVector<PatternType, 20> patternOrder;

void wait_for_serial_connection()
{
	uint32_t timeout_end = millis() + 2000;
	Serial.begin(9600);
	while (!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
}

void setup()
{
	wait_for_serial_connection(); // Optional, but seems to help Teensy out a lot.

	if (!BLE.begin())
	{
		Serial.println("Failed to initialize BLE");
		while (1) {};
	}

	// Note: ArduinoBLE library has limited security support
	// We'll rely on application-level authentication instead
	BLE.setLocalName("SRDriver");
	BLE.setAdvertisedService(authService);

	authService.addCharacteristic(authCharacteristic);

	// Add descriptors to characteristics
	authCharacteristic.addDescriptor(authDescriptor);

	BLE.addService(authService);

	authCharacteristic.writeValue("0");
	BLE.advertise();
	Serial.println("BLE initialized");


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
	Serial.println("Setup");

	patternOrder.push_back(PatternType::WAVE_PLAYER_PATTERN);


	// Initialize LightPlayer2
	// LtPlay2.init(LightArr[0], 8, 8, pattData[0], 2);
	// LtPlay2.update();

	Serial.println("Initializing wave player configs");
	initWaveData(wavePlayerConfigs[0]);
	initWaveData2(wavePlayerConfigs[1]);
	initWaveData3(wavePlayerConfigs[2]);
	initWaveData4(wavePlayerConfigs[3]);
	initWaveData5(wavePlayerConfigs[4]);
	initWaveData6(wavePlayerConfigs[5]);
	initWaveData7(wavePlayerConfigs[6]);
	initWaveData8(wavePlayerConfigs[7]);
	initWaveData9(wavePlayerConfigs[8]);

	SwitchWavePlayerIndex(0);

	Serial.println("Setup complete");
	pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
	pinMode(PUSHBUTTON_PIN_SECONDARY, INPUT_PULLUP);
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
float speedMultiplier = 8.0f;

fl::FixedVector<int, LEDS_MATRIX_Y> sharedIndices;
void UpdateAllCharacteristicsForCurrentPattern()
{
	// Update pattern index characteristic
	patternIndexCharacteristic.writeValue(String(currentWavePlayerIndex));

	// Update color characteristics based on current pattern
	const auto currentColors = GetCurrentPatternColors();
	String highColorStr = String(currentColors.first.r) + "," + String(currentColors.first.g) + "," + String(currentColors.first.b);
	String lowColorStr = String(currentColors.second.r) + "," + String(currentColors.second.g) + "," + String(currentColors.second.b);

	highColorCharacteristic.writeValue(highColorStr);
	lowColorCharacteristic.writeValue(lowColorStr);

	// Update brightness characteristic
	brightnessCharacteristic.writeValue(String(GlobalBrightness));

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

			leftSeriesCoefficientsCharacteristic.writeValue(leftCoeffsStr);
			rightSeriesCoefficientsCharacteristic.writeValue(rightCoeffsStr);
		}
	}
}

void GoToNextPattern()
{
	// Just reset everything
	// currentPatternIndex++;
	const auto currentPattern = patternOrder[currentPatternIndex % patternOrder.size()];
	currentWavePlayerIndex++;
	if (currentWavePlayerIndex >= numWavePlayerConfigs)
	{
		currentWavePlayerIndex = 0;
	}
	// If we are a wave player, call init on the wave player with the new index
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
	// currentPatternIndex = patternIndex;
	currentWavePlayerIndex = patternIndex;
	sharedCurrentIndexState = 0;
	Serial.println("GoToPattern" + String(currentWavePlayerIndex));
	UpdateAllCharacteristicsForCurrentPattern();
	SwitchWavePlayerIndex(currentWavePlayerIndex);
}

void IncrementSharedCurrentIndexState(unsigned int limit, unsigned int count = 1)
{
	sharedCurrentIndexState += count;
	if (!ONLY_PUSHBUTTON_PATTERN_CHANGE && sharedCurrentIndexState >= limit)
	{
		GoToNextPattern();
	}
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
		case PatternType::WAVE_PLAYER_PATTERN:
		{
			wavePlayer.update(wavePlayerSpeeds[currentWavePlayerIndex] * speedMultiplier);
			for (int i = 0; i < LEDS_MATRIX_1; ++i)
			{
				leds[i].r = LightArr[i].r;
				leds[i].g = LightArr[i].g;
				leds[i].b = LightArr[i].b;
			}
			IncrementSharedCurrentIndexState(wavePlayerLengths[currentWavePlayerIndex]);
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


	// largeWavePlayer.update(0.01f * speedMultiplier);

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
			LtPlay2.onLt = newHighLt;
			LtPlay2.offLt = newLowLt;
			// LtPlay2.init(LightArr[0], LtPlay2.rows, LtPlay2.cols, newHighLt, newLowLt);
			break;
	}

	// Update characteristics to reflect the new colors
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
			return std::make_pair(LtPlay2.onLt, LtPlay2.offLt);
		case PatternType::DATA_PATTERN:
		case PatternType::RING_PATTERN:
		case PatternType::COLUMN_PATTERN:
		case PatternType::ROW_PATTERN:
		case PatternType::DIAGONAL_PATTERN:
		default:
			return std::make_pair(Light(0, 0, 0), Light(0, 0, 0));
	}
}

void UpdateBrightnessInt(int value)
{
	GlobalBrightness = value;
	FastLED.setBrightness(GlobalBrightness);
}

void UpdateBrightness(float value)
{
	GlobalBrightness = value * 255;
	FastLED.setBrightness(GlobalBrightness);
}

void CheckPotentiometers()
{
	// Always check brightness potentiometer regardless of potensControlColor
	// Call getValue() to update the change detection state
	brightnessPot.getValue();

	if (brightnessPot.hasChanged())
	{
		Serial.println("Brightness potentiometer has changed");
		float brightness = brightnessPot.getCurveMappedValue();
		UpdateBrightness(brightness);
		brightnessCharacteristic.writeValue(String(brightness * 255));
		brightnessPot.resetChanged();
	}

	int speed = speedPot.getMappedValue(0, 255);
	int extra = extraPot.getMappedValue(0, 255);
	speedMultiplier = speed / 255.f * 20.f;
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

// Authentication helper functions
bool isDeviceAuthorized(const String &deviceAddress)
{
	for (int i = 0; i < numAuthorizedDevices; i++)
	{
		if (authorizedDevices[i] == deviceAddress)
		{
			return true;
		}
	}
	return false;
}

void addAuthorizedDevice(const String &deviceAddress)
{
	if (numAuthorizedDevices < MAX_AUTHORIZED_DEVICES && !isDeviceAuthorized(deviceAddress))
	{
		authorizedDevices[numAuthorizedDevices] = deviceAddress;
		numAuthorizedDevices++;
		Serial.println("Device authorized: " + deviceAddress);
	}
}

void removeAuthorizedDevice(const String &deviceAddress)
{
	for (int i = 0; i < numAuthorizedDevices; i++)
	{
		if (authorizedDevices[i] == deviceAddress)
		{
			// Shift remaining devices down
			for (int j = i; j < numAuthorizedDevices - 1; j++)
			{
				authorizedDevices[j] = authorizedDevices[j + 1];
			}
			numAuthorizedDevices--;
			Serial.println("Device removed: " + deviceAddress);
			break;
		}
	}
}

void enterPairingMode()
{
	pairingMode = true;
	pairingModeStartTime = millis();
	Serial.println("Entering pairing mode for 30 seconds");
	// Visual indicator - flash LEDs in pairing pattern
	for (int i = 0; i < NUM_LEDS; i++)
	{
		leds[i] = CRGB::Yellow;
	}
	FastLED.show();
	delay(500);
	FastLED.clear();
	FastLED.show();
}

void exitPairingMode()
{
	pairingMode = false;
	Serial.println("Exiting pairing mode");
}

void addControlService()
{
	if (!controlServiceAdded)
	{
		// Add control characteristics to the control service
		controlService.addCharacteristic(brightnessCharacteristic);
		controlService.addCharacteristic(patternIndexCharacteristic);
		controlService.addCharacteristic(highColorCharacteristic);
		controlService.addCharacteristic(lowColorCharacteristic);
		controlService.addCharacteristic(leftSeriesCoefficientsCharacteristic);
		controlService.addCharacteristic(rightSeriesCoefficientsCharacteristic);
		controlService.addCharacteristic(commandCharacteristic);

		// Add descriptors to control characteristics
		brightnessCharacteristic.addDescriptor(brightnessDescriptor);
		patternIndexCharacteristic.addDescriptor(patternIndexDescriptor);
		highColorCharacteristic.addDescriptor(highColorDescriptor);
		lowColorCharacteristic.addDescriptor(lowColorDescriptor);
		leftSeriesCoefficientsCharacteristic.addDescriptor(leftSeriesCoefficientsDescriptor);
		rightSeriesCoefficientsCharacteristic.addDescriptor(rightSeriesCoefficientsDescriptor);
		commandCharacteristic.addDescriptor(commandDescriptor);
		// Add format descriptors
		brightnessCharacteristic.addDescriptor(brightnessFormatDescriptor);
		patternIndexCharacteristic.addDescriptor(patternIndexFormatDescriptor);
		highColorCharacteristic.addDescriptor(highColorFormatDescriptor);
		lowColorCharacteristic.addDescriptor(lowColorFormatDescriptor);
		leftSeriesCoefficientsCharacteristic.addDescriptor(leftSeriesCoefficientsFormatDescriptor);
		rightSeriesCoefficientsCharacteristic.addDescriptor(rightSeriesCoefficientsFormatDescriptor);
		commandCharacteristic.addDescriptor(commandFormatDescriptor);
		// Add the service to BLE
		BLE.addService(controlService);

		// Update all characteristics with current values
		UpdateAllCharacteristicsForCurrentPattern();

		controlServiceAdded = true;
		Serial.println("Control service added after authentication");
	}
}

void HandleBLE()
{
	static bool connected = false;
	static String currentDeviceAddress = "";
	BLEDevice central = BLE.central();

	// Check pairing mode timeout
	if (pairingMode && (millis() - pairingModeStartTime > PAIRING_TIMEOUT))
	{
		exitPairingMode();
	}

	if (central)
	{
		if (!connected)
		{
			// Only print when we connect the first time
			Serial.print("Connected to central: ");
			Serial.println(central.address());
			currentDeviceAddress = String(central.address());

			// Reset authentication for new connection
			isAuthenticated = false;

			// Check if device is already authorized
			if (isDeviceAuthorized(currentDeviceAddress))
			{
				isAuthenticated = true;
				addControlService(); // Add control service for already authorized device
				Serial.println("Device already authorized");
			}
			else
			{
				Serial.println("Device not authorized - requires PIN");
			}
		}

		if (central.connected())
		{
			connected = true;

			// Handle authentication characteristic
			if (authCharacteristic.written())
			{
				const auto value = authCharacteristic.value();
				Serial.println("Auth characteristic written: " + String(value));

				if (pairingMode)
				{
					// In pairing mode, any PIN will authorize the device
					addAuthorizedDevice(currentDeviceAddress);
					isAuthenticated = true;
					authCharacteristic.writeValue("1"); // Success
					exitPairingMode();
					addControlService(); // Add control service after successful pairing
					Serial.println("Device paired successfully");
				}
				else if (String(value) == AUTH_PIN)
				{
					// Correct PIN provided
					addAuthorizedDevice(currentDeviceAddress);
					isAuthenticated = true;
					authCharacteristic.writeValue("1"); // Success
					addControlService(); // Add control service after successful authentication
					Serial.println("Authentication successful");
				}
				else
				{
					// Wrong PIN
					authCharacteristic.writeValue("0"); // Failure
					Serial.println("Authentication failed - wrong PIN");
				}
			}

			// Only process control characteristics if authenticated
			if (isAuthenticated)
			{
				if (brightnessCharacteristic.written())
				{
					const auto value = brightnessCharacteristic.value();
					Serial.println("Brightness characteristic written" + String(value));
					// String will be 0-255
					int val = value.toInt();
					Serial.println("Setting brightness to: " + String(val));
					UpdateBrightnessInt(val);
				}

				if (patternIndexCharacteristic.written())
				{
					const auto value = patternIndexCharacteristic.value();
					Serial.println("Pattern index characteristic written" + String(value));
					int val = value.toInt();
					Serial.println("Setting pattern index to: " + String(val));
					GoToPattern(val);
				}
				if (highColorCharacteristic.written())
				{
					WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
					if (currentWavePlayer)
					{
						UpdateColorFromCharacteristic(highColorCharacteristic, currentWavePlayer->hiLt, true);
					}
				}

				if (lowColorCharacteristic.written())
				{
					WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
					if (currentWavePlayer)
					{
						UpdateColorFromCharacteristic(lowColorCharacteristic, currentWavePlayer->loLt, false);
					}
				}

				if (leftSeriesCoefficientsCharacteristic.written())
				{
					WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
					if (currentWavePlayer)
					{
						Serial.println("Updating left series coefficients for current wave player");
						UpdateSeriesCoefficientsFromCharacteristic(leftSeriesCoefficientsCharacteristic, *currentWavePlayer);
					}
					else
					{
						Serial.println("No wave player available for series coefficients update");
					}
				}

				if (rightSeriesCoefficientsCharacteristic.written())
				{
					WavePlayer *currentWavePlayer = GetCurrentWavePlayer();
					if (currentWavePlayer)
					{
						Serial.println("Updating right series coefficients for current wave player");
						UpdateSeriesCoefficientsFromCharacteristic(rightSeriesCoefficientsCharacteristic, *currentWavePlayer);
					}
					else
					{
						Serial.println("No wave player available for series coefficients update");
					}
				}

				if (commandCharacteristic.written())
				{
					const auto value = commandCharacteristic.value();
					Serial.println("Command characteristic written: " + String(value));
					ParseAndExecuteCommand(value);
				}

			}
			else
			{
				// Not authenticated - ignore control commands
				if (brightnessCharacteristic.written() || patternIndexCharacteristic.written() ||
					highColorCharacteristic.written() || lowColorCharacteristic.written() ||
					leftSeriesCoefficientsCharacteristic.written() || rightSeriesCoefficientsCharacteristic.written() ||
					commandCharacteristic.written())
				{
					Serial.println("Control command ignored - not authenticated");
				}
			}
		}
		else
		{
			connected = false;
			Serial.println("Disconnected from central: ");
			Serial.println(central.address());
			isAuthenticated = false;
		}
	}
}

int loopCount = 0;
void loop()
{
	unsigned long ms = millis();
	FastLED.clear();
	Button::Event buttonEvent = pushButton.getEvent();
	Button::Event buttonEventSecondary = pushButtonSecondary.getEvent();

	// Primary button - pattern change
	if (buttonEvent == Button::Event::PRESS)
	{
		Serial.println("Primary button pressed");
		GoToNextPattern();
	}

	// Long press secondary button to enter pairing mode
	if (buttonEventSecondary == Button::Event::HOLD)
	{
		Serial.println("Secondary button long pressed - entering pairing mode");
		enterPairingMode();
	}

	// Handle BLE connections and authentication
	HandleBLE();

	// Visual feedback for authentication status
	if (pairingMode)
	{
		// Blinking yellow during pairing mode
		if ((ms / 500) % 2 == 0)
		{
			for (int i = 0; i < NUM_LEDS; i++)
			{
				leds[i] = CRGB::Yellow;
			}
		}
	}
	else if (!isAuthenticated)
	{
		// Red indicator when not authenticated
		for (int i = 0; i < NUM_LEDS; i++)
		{
			leds[i] = CRGB::Red;
		}
	}

	UpdatePattern(buttonEvent);
	CheckPotentiometers();
	UpdateBrightnessPulse();

	last_ms = ms;
	FastLED.show();
	delay(8.f);
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
	else
	{
		Serial.println("Unknown command: " + cmd);
	}
}

void StartBrightnessPulse(int targetBrightness, unsigned long duration)
{
	// Store current brightness before starting pulse
	previousBrightness = GlobalBrightness;
	pulseTargetBrightness = targetBrightness;
	pulseDuration = duration;
	pulseStartTime = millis();
	isPulsing = true;

	Serial.println("Starting brightness pulse from " + String(previousBrightness) + " to " + String(targetBrightness));
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
		// Pulse complete - return to previous brightness
		UpdateBrightnessInt(previousBrightness);
		isPulsing = false;
		Serial.println("Brightness pulse complete - returned to " + String(previousBrightness));
		return;
	}

	// Calculate current brightness using smooth interpolation
	float progress = (float) elapsed / (float) pulseDuration;

	// Use smooth sine wave interpolation for natural pulsing effect
	// This creates a full cycle: 0 -> 1 -> 0 over the duration
	float smoothProgress = (sin(progress * 2 * PI - PI / 2) + 1) / 2; // Full cycle: 0 to 1 to 0

	int currentBrightness = previousBrightness + (int) ((pulseTargetBrightness - previousBrightness) * smoothProgress);

	// Apply the calculated brightness
	UpdateBrightnessInt(currentBrightness);
}
