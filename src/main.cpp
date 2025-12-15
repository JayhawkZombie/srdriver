#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0
// #define FASTLED_RP2040_CLOCKLESS_PIO 0

#include <FastLED.h>
#include <stdint.h>
#include "Utils.hpp"
#include "Globals.h"
#include "../lights/LightPlayer2.h"
#include "../lights/DataPlayer.h"
#include "hal/input/buttons/Button.hpp"
#include "hal/input/potentiometers/Potentiometer.hpp"
#include "die.hpp"
#include "../lights/WavePlayer.h"

// Platform configuration and HAL
#include "PlatformConfig.h"
#include "hal/PlatformFactory.h"
#include "hal/SDCardController.h"

// Conditional includes based on platform support
#if SUPPORTS_BLE
#include <ArduinoBLE.h>
#endif

#include "GlobalState.h"
#include "DeviceState.h"
#include "hal/ble/BLEManager.h"
#include "PatternManager.h"
#include "UserPreferences.h"
#include "controllers/BrightnessController.h"
#include "controllers/SpeedController.h"

#include <array>
#include <memory>
#include <vector>

// FreeRTOS includes
#include "freertos/SRTask.h"
#if SUPPORTS_SD_CARD
#include "freertos/LogManager.h"
#endif
#include "freertos/LEDUpdateTask.h"
#include "freertos/BLEUpdateTask.h"
#include "freertos/WiFiManager.h"
#include "freertos/SystemMonitorTask.h"

#if SUPPORTS_SD_CARD
#include "utility/SDUtils.h"
#include "utility/OutputManager.h"
#include "hal/SDCardAPI.h"
#include "hal/FileParser.h"
#endif

#if SUPPORTS_DISPLAY
#include "hal/display/SSD_1306Component.h"
#include "freertos/DisplayTask.h"
SSD1306_Display display;
#endif
#include "config/JsonSettings.h"
#include "utility/StringUtils.h"


#include "freertos/HardwareInputTask.h"
#include "hal/input/InputEvent.h"
#include "lights/LEDManager.h"

// Global FreeRTOS task instances
static LEDUpdateTask *g_ledUpdateTask = nullptr;
#if SUPPORTS_BLE
static BLEUpdateTask *g_bleUpdateTask = nullptr;
#endif
static WiFiManager *g_wifiManager = nullptr;
static SystemMonitorTask *g_systemMonitorTask = nullptr;
#if SUPPORTS_DISPLAY
static DisplayTask *g_displayTask = nullptr;
#endif

HardwareInputTask *g_hardwareInputTask = nullptr;

// Global HAL instances
#if SUPPORTS_SD_CARD
SDCardController *g_sdCardController = nullptr;
#endif
JsonSettings settings("/config/settings.json");
bool settingsLoaded = false;

#if SUPPORTS_TEMPERATURE_SENSOR
DS18B20Component *g_temperatureSensor = nullptr;
#endif

// Global power sensor instances
ACS712CurrentSensor *g_currentSensor = nullptr;
ACS712VoltageSensor *g_voltageSensor = nullptr;


#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
Rgbw rgbw = Rgbw(
	kRGBWDefaultColorTemp,
	kRGBWExactColors,      // Mode
	W3                     // W-placement
);

typedef WS2812<LED_PIN, RGB> ControllerT;  // RGB mode must be RGB, no re-ordering allowed.
static RGBWEmulatedController<ControllerT, GRB> rgbwEmu(rgbw);  // ordering goes here.
#endif

CRGB leds[NUM_LEDS];

// Global SD card availability flag
#if SUPPORTS_SD_CARD
bool g_sdCardAvailable = false;
#endif

#include "../hal/input/switchSPST.h"
int pinEncoderCLK = D4, pinEncoderDT = D3, pinEncoderSW = D2;
int lastClkState = HIGH;
void updateEncoder();

switchSPST rotEncButton;

volatile bool isShuttingDown = false;

void OnSettingChanged(DeviceState &state)
{
	Serial.println("Device state changed");
	LOG_INFOF_COMPONENT("Main", "Device state changed: brightness: %d", state.brightness);
	FastLED.setBrightness(state.brightness);
	SaveUserPreferences(state);
	// Optionally: save preferences, update UI, etc.
}

void ShowStartupStatusMessage(String message)
{
#if SUPPORTS_DISPLAY
	// Show into a buffer so we see: Startup: [message]
	char buffer[100];
	snprintf(buffer, sizeof(buffer), "Startup: %s", message.c_str());
	buffer[sizeof(buffer) - 1] = '\0';
	display.clear();
	display.setTextColor(COLOR_WHITE);
	display.setTextSize(1);
	display.printCentered(2, "SRDriver", 1);
	display.drawLine(0, 12, 128, 12, COLOR_WHITE);
	display.printAt(2, 20, buffer, 1);
	display.show();
#endif
}

void wait_for_serial()
{
	while (!Serial)
	{
		delay(100);
	}
}

void SetupOthers()
{
	pinMode(pinEncoderCLK, INPUT_PULLUP);
	pinMode(pinEncoderDT, INPUT_PULLUP);
	// pinMode(pinEncoderSW, INPUT_PULLUP);
	rotEncButton.init(pinEncoderSW, 0.3f, false);
	lastClkState = digitalRead(pinEncoderCLK);// initial value
	attachInterrupt(digitalPinToInterrupt(pinEncoderCLK), updateEncoder, CHANGE);
}

// We want a full rotation to be one change, but this is actually a change
// of 2 steps, a change occurs on a halfway step as well, which is in-between
// the notched points in the encoder.
volatile bool didChangeValue = false;
volatile int encoderValue = 0;
bool didChangeUp = false;

void updateEncoder()
{
	int currentClkState = digitalRead(pinEncoderCLK);
	int currentDtState = digitalRead(pinEncoderDT);
	const auto lastEncoderValue = encoderValue;

	if (currentClkState != lastClkState)
	{ // Check for a change in CLK
		if (currentDtState != currentClkState)
		{ // Determine direction
			encoderValue++;
			didChangeUp = true;
		}
		else
		{
			encoderValue--;
			didChangeUp = false;
		}
	}

	if (encoderValue != lastEncoderValue)
	{
		didChangeValue = true;
	}

	lastClkState = currentClkState;
}

int encoderBrightness = 128;

void UpdateBrightnessFromEncoder()
{
	BrightnessController *brightnessController = BrightnessController::getInstance();
	if (!brightnessController) {
		return;
	}

	int brightness = brightnessController->getBrightness();
	if (didChangeUp) {
		encoderBrightness++;
		encoderBrightness = constrain(encoderBrightness, 0, 255);
		brightnessController->updateBrightness(encoderBrightness);
	} else {
		encoderBrightness--;
		encoderBrightness = constrain(encoderBrightness, 0, 255);
		brightnessController->updateBrightness(encoderBrightness);
	}
}

// Order of effects to switch through when clicking the rotary encoder button
const std::vector<String> effectOrderJsonStrings = {
	// They're just effect commands, the same as would be sent to the WebSocket server
	{
		R"delimiter(
		{
			"t": "effect",
			"e": {
				"t": "white",
				"p": {
					"s": 1.0,
					"r": false,
					"d": -1
				}
			}
		}
		)delimiter",
	},
	{
		R"delimiter(
		{
			"t": "effect",
			"e": {
				"t": "rainbow",
				"p": {
					"s": 1.0,
					"r": false,
					"d": -1
				}
			}
		}
		)delimiter",
	},
	{
		R"delimiter(
		{
			"t": "effect",
			"e": {
				"t": "color_blend",
				"p": {
					"c1": "rgb(255,0,0)",
					"c2": "rgb(0,255,0)",
					"s": 1.0,
					"d": -1
				}
			}
		}
		)delimiter",
	},
	{
		R"delimiter(
		{
			"t": "effect",
			"e": {
				"t": "twinkle",
				"p": {
					"mnd": 0.5,
					"mxd": 5.6,
					"mns": 0.5,
					"mxs": 5.6,
					"sc": 0.4,
					"mb": 100,
					"fis": 4,
					"fos": 2
				}
			}
		}
		)delimiter",
	}
};
int currentEffectIndex = 0;

void TriggerNextEffect()
{
	currentEffectIndex++;
	if (currentEffectIndex >= effectOrderJsonStrings.size()) {
		currentEffectIndex = 0;
	}
	LOG_DEBUGF_COMPONENT("Main", "Triggering next effect: (%i) %s", effectOrderJsonStrings[currentEffectIndex].length(), effectOrderJsonStrings[currentEffectIndex].c_str());
	String effectCommandJsonString = effectOrderJsonStrings[currentEffectIndex];
	HandleJSONCommand(effectCommandJsonString);
}

void LoopOthers(float dt)
{
	rotEncButton.update(dt);
	if (rotEncButton.pollEvent() == 1)
	{
		// PRESSED
		LOG_DEBUGF_COMPONENT("Main", "Rotary encoder button pressed");
		LOG_DEBUGF_COMPONENT("Main", "Current effect index: %d", currentEffectIndex);

		// Try triggering the next effect?
		TriggerNextEffect();
	} else if (rotEncButton.pollEvent() == -1)
	{
		// RELEASED
		LOG_DEBUGF_COMPONENT("Main", "Rotary encoder button released");
	}

	if (didChangeValue)
	{
		didChangeValue = false;
		LOG_DEBUGF_COMPONENT("Main", "Rotary encoder value changed: %d", encoderValue);
		UpdateBrightnessFromEncoder();
	}
}

// Skip setting brightness from user settings if we're using the hardware input task
bool skipBrightnessFromUserSettings = false;

// Function to register all BLE characteristics
void registerAllBLECharacteristics()
{
	BLEManager *ble = BLEManager::getInstance();
	if (!ble)
	{
		LOG_ERROR_COMPONENT("Startup", "BLE not available");
		return;
	}

	LOG_INFO_COMPONENT("Startup", "Registering all BLE characteristics...");

	// Initialize and register brightness controller
	LOG_INFO_COMPONENT("Startup", "Initializing BrightnessController...");
	BrightnessController::initialize();

	BrightnessController *brightnessController = BrightnessController::getInstance();
	if (brightnessController)
	{
		LOG_INFO_COMPONENT("Startup", "Brightness controller already initialized, registering characteristic...");
		brightnessController->registerBLECharacteristic();
		LOG_INFO_COMPONENT("Startup", "Brightness characteristic registration complete");
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Brightness controller not available for BLE registration");
	}

	// Initialize and register speed controller
	SpeedController::initialize();
	SpeedController *speedController = SpeedController::getInstance();
	if (speedController)
	{
		LOG_INFO_COMPONENT("Startup", "Speed controller initialized, registering characteristic...");
		speedController->registerBLECharacteristic();
		LOG_INFO_COMPONENT("Startup", "Speed controller characteristic registration complete");
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to initialize speed controller");
	}

	// In the future, we can move these to their respective controllers
	// SpeedController::registerBLECharacteristics();
	// PatternController::registerBLECharacteristics();

	LOG_INFO_COMPONENT("Startup", "All characteristics registered");
}

void SerialAwarePowerLimiting()
{
	// Many computers will complain about power consumption, and sometimes
	// will just outright disable the USB device, so we'll attempt to limit
	// power if we can detect a USB serial connection (not the best check,
	// since it'll miss most cases, but it's better than nothing)
	if (Serial)
	{
		FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
	}
}

void OnShutdown()
{
	LOG_INFO_COMPONENT("Main", "OnShutdown called");
	isShuttingDown = true;
	for (auto &led : leds)
	{
		led = CRGB::Black;
	}
	FastLED.setBrightness(0);
	FastLED.clear();
	FastLED.show();
	digitalWrite(LED_PIN, LOW);
	// esp_restart();
}

void TryParseJson()
{
	if (g_sdCardController && g_sdCardController->isAvailable()) {
		String jsonString = g_sdCardController->readFile("/SD_init.json");
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, jsonString);
		if (error) {
			LOG_ERRORF_COMPONENT("Main", "Failed to deserialize JSON: %s", error.c_str());
			return;
		}
		if (doc.containsKey("files")) {
			JsonArray files = doc["files"];
			for (JsonVariant file : files) {
				String fileName = file.as<String>();
				LOG_INFOF_COMPONENT("Main", "File: %s", fileName.c_str());
			}
		} else {
			LOG_ERRORF_COMPONENT("Main", "No files key found in JSON");
		}
	} else {
		LOG_ERROR_COMPONENT("Main", "SD card controller not available - cannot try to parse JSON");
	}
}

void setup()
{
	// wait_for_serial();
	// MOVED: FastLED setup to beginning of setup() to
	// make sure it's blacked out until we're ready to use it
	// Used for RGB (NOT RGBW) LED strip
#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
	FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
#else
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#endif

	// Black out the LEDs to start with
	for (auto &led : leds)
	{
		led = CRGB::Black;
	}
	FastLED.clear();
	FastLED.show();

	esp_register_shutdown_handler(OnShutdown);

	Serial.begin(9600);
	SerialAwarePowerLimiting();
	SetupOthers();
	// LOG_INFO_COMPONENT("Startup", "Beginning setup");
	LOG_INFOF_COMPONENT("Startup", "Platform: %s", PlatformFactory::getPlatformName());

	// Initialize platform HAL
#if SUPPORTS_SD_CARD
	g_sdCardController = PlatformFactory::createSDCardController();
#endif

#if SUPPORTS_TEMPERATURE_SENSOR
	g_temperatureSensor = PlatformFactory::createTemperatureSensor(ONE_WIRE_BUS);
	g_temperatureSensor->begin();
#endif

	// Initialize SD card using HAL
#if SUPPORTS_SD_CARD
	g_sdCardAvailable = g_sdCardController->begin(SDCARD_PIN);
	if (!g_sdCardAvailable)
	{
		LOG_WARN_COMPONENT("Startup", "SD card not available - continuing without SD card support");
	}
	else
	{
		LOG_INFO_COMPONENT("Startup", "SD card initialized successfully");
	}

	SDCardAPI::initialize();
	delay(100);
#endif

	ShowStartupStatusMessage("FreeRTOS Logging");

	// Initialize FreeRTOS logging system
	// LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS logging system...");
#if SUPPORTS_SD_CARD
	LogManager::getInstance().initialize();

	// Configure log filtering (optional - can be enabled/disabled)
	// Uncomment the line below to show only WiFiManager logs:
	std::vector<String> logFilters = { "Startup", "WiFiManager", "WebSocketServer", "Main", "PulsePlayerEffect"};
	LOG_SET_COMPONENT_FILTER(logFilters);

	// Uncomment the line below to show only new logs (filter out old ones):
	// LOG_SET_NEW_LOGS_ONLY();

	// LOG_INFO_COMPONENT("Startup", "FreeRTOS logging system initialized");
	// LOG_INFOF_COMPONENT("Startup", "System started at: %d ms", millis());
	// LOG_INFOF_COMPONENT("Startup", "SD card available: %s", g_sdCardAvailable ? "yes" : "no");
#else
	LOG_INFO_COMPONENT("Startup", "FreeRTOS logging system started (SD card not supported)");
#endif

	// g_hardwareInputTask = HardwareInputTaskBuilder()
	// 	.build();

	// if (g_hardwareInputTask && g_hardwareInputTask->start())
	// {
	// 	LOG_INFO_COMPONENT("Startup", "Hardware input task started");
	// }
	// else
	// {
	// 	LOG_ERROR_COMPONENT("Startup", "Failed to start hardware input task");
	// }

#if SUPPORTS_SD_CARD
	// NOW we can load the settings??????
	// LOG_DEBUG_COMPONENT("Startup", "Loading settings");
	settingsLoaded = settings.load();

	// if (!settingsLoaded)
	// {
	// 	LOG_ERROR_COMPONENT("Startup", "Failed to load settings");
	// }
#endif

// TryParseJson();

#if SUPPORTS_DISPLAY
	if (settingsLoaded)
	{
		const auto displaySettings = settings._doc["display"];
		if (!displaySettings.isNull())
		{
			const auto addressSetting = displaySettings["address"];
			if (!addressSetting.isNull())
			{
				uint8_t address = hexToUint8(addressSetting.as<String>());
				// LOG_DEBUGF_COMPONENT("Startup", "Found Display address: %d", address);

				display.setAddress(address);
				// LOG_DEBUGF_COMPONENT("Startup", "Display address set to: %d", address);
			}
		}
	}

	display.setupDisplay();

	// Initialize DisplayQueue in STARTUP state
	DisplayQueue::getInstance().setDisplayState(DisplayQueue::DisplayState::STARTUP);

	// Immediately render to the display (Cannot use DisplayTask or DisplayQueue
	// yet because they are not initialized)
	ShowStartupStatusMessage("Starting");
#endif

#if SUPPORTS_BLE
	BLEManager *bleManager = nullptr;
	ShowStartupStatusMessage("BLE");
	if (!BLE.begin())
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to initialize BLE, continuing without BLE support");
	}
	else
	{
		BLE.setLocalName("SRDriver");
		BLE.setDeviceName("SRDriver");
		BLE.advertise();
		LOG_INFO_COMPONENT("Startup", "BLE initialized");

		BLEManager::initialize(deviceState, GoToPattern);
		bleManager = BLEManager::getInstance();
		
		if (bleManager)
		{
			// Register characteristics BEFORE starting BLE
			bleManager->registerCharacteristics();
			bleManager->getHeartbeatCharacteristic().writeValue(millis());
			// Register all additional BLE characteristics
			registerAllBLECharacteristics();
			// Now start BLE advertising
			bleManager->begin();
			bleManager->setOnSettingChanged(OnSettingChanged);

			// Initialize FreeRTOS BLE update task
			LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS BLE update task...");
			// Reuse the existing bleManager variable
			if (bleManager)
			{
				g_bleUpdateTask = new BLEUpdateTask(*bleManager);
				if (g_bleUpdateTask->start())
				{
					// LOG_INFO_COMPONENT("Startup", "FreeRTOS BLE update task started");
				}
				else
				{
					// LOG_ERROR_COMPONENT("Startup", "Failed to start FreeRTOS BLE update task");
				}
			}
			else
			{
				// LOG_ERROR_COMPONENT("Startup", "BLE not available - cannot start BLE update task");
			}
		}
		else
		{
			// LOG_ERROR_COMPONENT("Startup", "BLEManager is null!");
		}
	}
#else
	LOG_INFO_COMPONENT("Startup", "BLE not supported on this platform");
#endif

	// Initialize WiFi manager
	LOG_INFO_COMPONENT("Startup", "Initializing WiFi manager...");
	g_wifiManager = new WiFiManager();
	if (g_wifiManager->start())
	{
		LOG_INFO_COMPONENT("Startup", "WiFi manager started");
		// Set BLE manager reference if available
#if SUPPORTS_BLE
		if (bleManager)
		{
			g_wifiManager->setBLEManager(bleManager);
			bleManager->setWiFiManager(g_wifiManager);
		}
#endif
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to start WiFi manager");
	}

	ShowStartupStatusMessage("Patterns");
	Pattern_Setup();

	extern LEDManager *g_ledManager;
	if (g_ledManager && g_wifiManager)
	{
		g_wifiManager->setLEDManager(g_ledManager);
		LOG_DEBUG_COMPONENT("Startup", "WiFiManager: LEDManager reference set for WebSocket");
	}


#if SUPPORTS_PREFERENCES
	prefsManager.begin();
	// LOG_DEBUG_COMPONENT("Startup", "Loading user preferences...");
	prefsManager.load(deviceState);
	// LOG_DEBUGF_COMPONENT("Startup", "Preferences loaded - WiFi SSID: '%s' (length: %d), Password length: %d",
	// 	deviceState.wifiSSID.c_str(), deviceState.wifiSSID.length(), deviceState.wifiPassword.length());
	prefsManager.end();
	ApplyFromUserPreferences(deviceState, skipBrightnessFromUserSettings);
	encoderBrightness = deviceState.brightness;
	// Load WiFi credentials and attempt connection
	// LOG_DEBUGF_COMPONENT("Startup", "Checking WiFi credentials - SSID length: %d, Password length: %d", deviceState.wifiSSID.length(), deviceState.wifiPassword.length());
	if (g_wifiManager && deviceState.wifiSSID.length() > 0)
	{
		// LOG_DEBUGF_COMPONENT("Startup", "Loading saved WiFi credentials for '%s'", deviceState.wifiSSID.c_str());
		// LOG_DEBUGF_COMPONENT("Startup", "WiFi SSID: '%s', Password length: %d", deviceState.wifiSSID.c_str(), deviceState.wifiPassword.length());
		g_wifiManager->setCredentials(deviceState.wifiSSID, deviceState.wifiPassword);
		// Trigger auto-connect attempt
		// LOG_DEBUG_COMPONENT("Startup", "WiFiManager: Calling checkSavedCredentials() to trigger auto-connect");
		g_wifiManager->checkSavedCredentials();
	}
	else
	{
		// LOG_DEBUGF_COMPONENT("Startup", "No WiFi credentials found - SSID length: %d", deviceState.wifiSSID.length());
	}
#else
	LOG_INFO("Preferences not supported on this platform - using defaults");
#endif

	// Initialize FreeRTOS LED update task
	// LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS LED update task...");
	g_ledUpdateTask = new LEDUpdateTask(16);  // 60 FPS
	if (g_ledUpdateTask->start())
	{
		// LOG_INFO_COMPONENT("Startup", "FreeRTOS LED update task started");
	}
	else
	{
		// LOG_ERROR_COMPONENT("Startup", "Failed to start FreeRTOS LED update task");
	}

#if SUPPORTS_POWER_SENSORS
	// Initialize global power sensors BEFORE creating SystemMonitorTask
	LOG_INFO("Initializing global power sensors...");
	LOG_WARN("IMPORTANT: Ensure LEDs are OFF during sensor calibration!");

	g_currentSensor = new ACS712CurrentSensor(A2, ACS712_30A, 5.0f, 3.3f);
	g_currentSensor->begin();
	g_currentSensor->setPolarityCorrection(false);

	g_voltageSensor = new ACS712VoltageSensor(A3, 3.3f, 5.27f);
	g_voltageSensor->begin();

	LOG_INFO("Global power sensors initialized successfully");

#if ENABLE_POWER_SENSOR_CALIBRATION_DELAY
	// Delay LED startup to allow power sensor calibration
	LOG_INFO("Power sensors detected - delaying LED startup for calibration...");
	LOG_INFO("Waiting 5 seconds for stable power sensor readings...");
	delay(5000); // 5 second delay for calibration

	// Force recalibration to ensure clean baseline
	if (g_currentSensor)
	{
		LOG_INFO("Forcing power sensor recalibration...");
		g_currentSensor->forceRecalibration();
		LOG_INFO("Power sensor calibration complete");
	}
#endif
#else
	// LOG_INFO_COMPONENT("Startup", "Power sensors not supported on this platform");
#endif

	// Initialize FreeRTOS system monitor task
	// LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS system monitor task...");
	g_systemMonitorTask = new SystemMonitorTask(15000);  // Every 15 seconds
	if (g_systemMonitorTask->start())
	{
		// LOG_INFO_COMPONENT("Startup", "FreeRTOS system monitor task started");
	}
	else
	{
		// LOG_ERROR_COMPONENT("Startup", "Failed to start FreeRTOS system monitor task");
	}

	// Initialize FreeRTOS display task
	// LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS display task...");
	g_displayTask = new DisplayTask(33);  // 30 FPS, we can't get better as there is a 25ms constant render time
	if (g_displayTask->start())
	{
		// LOG_INFO_COMPONENT("Startup", "FreeRTOS display task started");
		// DisplayTask will set state to READY when it starts running
	}
	else
	{
		// LOG_ERROR_COMPONENT("Startup", "Failed to start FreeRTOS display task");
		DisplayQueue::getInstance().setDisplayState(DisplayQueue::DisplayState::ERROR);
	}

	ShowStartupStatusMessage("Done");

	// // Log the final display system state
	// DisplayQueue::DisplayState finalState = DisplayQueue::getInstance().getDisplayState();
	// switch (finalState)
	// {
	// 	case DisplayQueue::DisplayState::STARTUP:
	// 		LOG_WARN_COMPONENT("Startup", "Display system still in STARTUP state - DisplayTask may not have started");
	// 		break;
	// 	case DisplayQueue::DisplayState::READY:
	// 		LOG_INFO_COMPONENT("Startup", "Display system ready - queue requests now accepted");
	// 		break;
	// 	case DisplayQueue::DisplayState::ERROR:
	// 		LOG_ERROR_COMPONENT("Startup", "Display system failed to start - queue requests will be ignored");
	// 		break;
	// }

	// WE're done!
	isBooting = false;
	LOG_INFO_COMPONENT("Startup", "Setup complete");
}

/**
 * Clean up FreeRTOS tasks
 * Call this during shutdown or restart
 */
void cleanupFreeRTOSTasks()
{
	// LOG_INFO("Shutting down FreeRTOS tasks...");

	// Stop and cleanup LED update task
	if (g_ledUpdateTask)
	{
		g_ledUpdateTask->stop();
		delete g_ledUpdateTask;
		g_ledUpdateTask = nullptr;
		// LOG_INFO("LED update task stopped");
	}

	// Stop and cleanup BLE update task
#if SUPPORTS_BLE
	if (g_bleUpdateTask)
	{
		g_bleUpdateTask->stop();
		delete g_bleUpdateTask;
		g_bleUpdateTask = nullptr;
		// LOG_INFO("BLE update task stopped");
	}
#endif

	// Stop and cleanup system monitor task
	if (g_systemMonitorTask)
	{
		g_systemMonitorTask->stop();
		delete g_systemMonitorTask;
		g_systemMonitorTask = nullptr;
		// LOG_INFO("System monitor task stopped");
	}

	// Stop and cleanup display task
#if SUPPORTS_DISPLAY
	if (g_displayTask)
	{
		g_displayTask->stop();
		delete g_displayTask;
		g_displayTask = nullptr;
		// LOG_INFO("Display task stopped");
	}
#endif

	// Cleanup SDCardAPI
#if SUPPORTS_SD_CARD
	SDCardAPI::cleanup();
	// LOG_INFO("SDCardAPI cleaned up");
#endif

	// Stop and cleanup SD writer task (flush logs first)
#if SUPPORTS_SD_CARD
	// This section is removed as per the edit hint.
#endif

	// LOG_INFO("FreeRTOS tasks cleanup complete");
}

void DrawError(const CRGB &color)
{
	for (int i = 0; i < LEDS_MATRIX_X; i += 2)
	{
		leds[i] = color;
	}
}

void loop()
{
	static unsigned long lastLoopTime = millis();
	const auto now = millis();
	const auto dt = now - lastLoopTime;
	lastLoopTime = now;
	float dtSeconds = dt * 0.001f;
	// int val5 = digitalRead(D2);
	// int val6 = digitalRead(D3);
	// int val7 = digitalRead(D4);

	// // Print values
	// Serial.printf("BUTTONS:%d, %d, %d\n", val5, val6, val7);

	// Update brightness controller
	BrightnessController *brightnessController = BrightnessController::getInstance();
	if (brightnessController)
	{
		brightnessController->update();
	}

	// Update speed controller
	SpeedController *speedController = SpeedController::getInstance();
	if (speedController)
	{
		speedController->update();
	}

	// Monitor FreeRTOS tasks every 5 seconds
	static unsigned long lastLogCheck = 0;
	LoopOthers(0.16f);
	if (now - lastLogCheck > 5000)
	{
		lastLogCheck = now;

		// Check FreeRTOS LED update task
		if (g_ledUpdateTask)
		{
			if (!g_ledUpdateTask->isRunning())
			{
				// LOG_ERROR("FreeRTOS LED update task stopped unexpectedly");
			}
			else
			{
				// LOG_DEBUGF("LED Update - Frames: %d, Interval: %d ms",
				// 	g_ledUpdateTask->getFrameCount(),
				// 	g_ledUpdateTask->getUpdateInterval());
			}
		}

		// Check FreeRTOS BLE update task
#if SUPPORTS_BLE
		if (g_bleUpdateTask && !g_bleUpdateTask->isRunning())
		{
			// LOG_ERROR("FreeRTOS BLE update task stopped unexpectedly");
		}
#endif

		// Check FreeRTOS system monitor task
		if (g_systemMonitorTask && !g_systemMonitorTask->isRunning())
		{
			// LOG_ERROR("FreeRTOS system monitor task stopped unexpectedly");
		}

		// Check FreeRTOS display task
#if SUPPORTS_DISPLAY
		// if (g_displayTask)
		// {
		// 	if (!g_displayTask->isRunning())
		// 	{
		// 		LOG_ERROR("FreeRTOS display task stopped unexpectedly");
		// 	}
		// 	else
		// 	{
		// 		LOG_DEBUGF("Display Update - Frames: %d, Interval: %d ms",
		// 			g_displayTask->getFrameCount(),
		// 			g_displayTask->getUpdateInterval());

		// 		// Check display performance
		// 		if (!g_displayTask->isPerformanceAcceptable())
		// 		{
		// 			LOG_WARNF("Display performance issue: %s", g_displayTask->getPerformanceReport().c_str());
		// 			LOG_INFO("Consider reducing display update frequency if performance issues persist");
		// 		}
		// 		else
		// 		{
		// 			LOG_DEBUGF("Display performance: %s", g_displayTask->getPerformanceReport().c_str());
		// 		}
		// 	}
		// }
#endif

		// Log detailed task information every 30 seconds
		static unsigned long lastDetailedCheck = 0;
		if (now - lastDetailedCheck > 30000)
		{
			lastDetailedCheck = now;
			if (g_systemMonitorTask)
			{
				g_systemMonitorTask->logDetailedTaskInfo();
			}
		}
	}

	// Serial command to trigger file streaming
	if (Serial.available())
	{
		String cmd = Serial.readStringUntil('\n');
		cmd.trim();

		// Log the command using new FreeRTOS logging
		LOG_INFO("Serial command received: " + cmd);

		// Handle power sensor recalibration command
#if SUPPORTS_POWER_SENSORS
		if (cmd == "recalibrate_power")
		{
			LOG_INFO("Force recalibrating power sensors...");
			if (g_currentSensor)
			{
				if (g_currentSensor->forceRecalibration())
				{
					LOG_INFO("Power sensor recalibration successful");
				}
				else
				{
					LOG_ERROR("Power sensor recalibration failed");
				}
			}
			else
			{
				LOG_ERROR("Current sensor not available for recalibration");
			}
			return; // Don't pass to SDCardAPI
		}

		// Handle clear calibration command
		if (cmd == "clear_calibration")
		{
			LOG_INFO("Clearing saved power sensor calibration...");
			Preferences prefs;
			if (prefs.begin("acs712_cal", false))
			{
				prefs.remove("midpoint");
				prefs.end();
				LOG_INFO("Saved calibration cleared - will recalibrate on next boot");
			}
			else
			{
				LOG_ERROR("Failed to clear calibration");
			}
			return; // Don't pass to SDCardAPI
		}
#else
		if (cmd == "recalibrate_power" || cmd == "clear_calibration")
		{
			// LOG_WARN("Power sensor commands not supported on this platform");
			return; // Don't pass to SDCardAPI
		}
#endif

		// Set output target to SERIAL_OUTPUT for commands received via serial
#if SUPPORTS_SD_CARD
		SDCardAPI::getInstance().setOutputTarget(OutputTarget::SERIAL_OUTPUT);
		SDCardAPI::getInstance().handleCommand(cmd);
		// Reset to BLE for future BLE commands
		SDCardAPI::getInstance().setOutputTarget(OutputTarget::BLE);
#endif
	}
}
