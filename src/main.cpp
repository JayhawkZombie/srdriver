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

// Skip setting brightness from user settings if we're using the hardware input task
bool skipBrightnessFromUserSettings = false;

// Function to register all BLE characteristics
void registerAllBLECharacteristics() {
    BLEManager* ble = BLEManager::getInstance();
    if (!ble) {
		LOG_ERROR_COMPONENT("Startup", "BLE not available");
        return;
    }
    
    LOG_INFO_COMPONENT("Startup", "Registering all BLE characteristics...");
    
    // Initialize and register brightness controller
	LOG_INFO_COMPONENT("Startup", "Initializing BrightnessController...");
	BrightnessController::initialize();

    BrightnessController* brightnessController = BrightnessController::getInstance();
    if (brightnessController) {
        LOG_INFO_COMPONENT("Startup", "Brightness controller already initialized, registering characteristic...");
        brightnessController->registerBLECharacteristic();
        LOG_INFO_COMPONENT("Startup", "Brightness characteristic registration complete");
    } else {
        LOG_ERROR_COMPONENT("Startup", "Brightness controller not available for BLE registration");
    }
    
    // Initialize and register speed controller
    SpeedController::initialize();
    SpeedController* speedController = SpeedController::getInstance();
    if (speedController) {
        LOG_INFO_COMPONENT("Startup", "Speed controller initialized, registering characteristic...");
        speedController->registerBLECharacteristic();
        LOG_INFO_COMPONENT("Startup", "Speed controller characteristic registration complete");
    } else {
        LOG_ERROR_COMPONENT("Startup", "Failed to initialize speed controller");
    }
    
    // In the future, we can move these to their respective controllers
    // SpeedController::registerBLECharacteristics();
    // PatternController::registerBLECharacteristics();
    
    LOG_INFO_COMPONENT("Startup", "All characteristics registered");
}

void setup()
{
	// wait_for_serial();
	Serial.begin(9600);
	LOG_INFO_COMPONENT("Startup", "Beginning setup");
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
	LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS logging system...");
#if SUPPORTS_SD_CARD
	LogManager::getInstance().initialize();
	
	// Configure log filtering (optional - can be enabled/disabled)
	// Uncomment the line below to show only WiFiManager logs:
	std::vector<String> logFilters = {"LEDManager", "WiFiManager", "WebSocketServer"};
	LOG_SET_COMPONENT_FILTER(logFilters);
	
	// Uncomment the line below to show only new logs (filter out old ones):
	// LOG_SET_NEW_LOGS_ONLY();
	
	LOG_INFO_COMPONENT("Startup", "FreeRTOS logging system started");

	// pinMode(D2, INPUT_PULLUP);  // D2 -> GPIO5
	// pinMode(D3, INPUT_PULLUP);  // D3 -> GPIO6
	// pinMode(D4, INPUT_PULLUP);  // D4 -> GPIO7

	// Test logging
	LOG_INFO_COMPONENT("Startup", "FreeRTOS logging system initialized");
	LOG_INFOF_COMPONENT("Startup", "System started at: %d ms", millis());
	LOG_INFOF_COMPONENT("Startup", "SD card available: %s", g_sdCardAvailable ? "yes" : "no");
#else
	LOG_INFO_COMPONENT("Startup", "FreeRTOS logging system started (SD card not supported)");
#endif

	// g_hardwareInputTask = HardwareInputTaskBuilder()
	// 	.fromJson("/data/hardwaredevices.json")
	// 	.build();

	g_hardwareInputTask = HardwareInputTaskBuilder()
		.addButton("touchButton1", D2, 50)
		.addButton("touchButton2", D3, 50)
		.addButton("touchButton3", D4, 50)
		// .addSlidePotentiometer("pot1", A6, 100, 4, 3, 3)  // Pin A0, 100ms poll, bitShift=3, minDiff=2, bumpLimit=3
		// .addSlidePotentiometer("pot2", A7, 100, 4, 3, 3)  // Pin A1, 100ms poll, bitShift=3, minDiff=1, bumpLimit=2
		.build();

	if (g_hardwareInputTask && g_hardwareInputTask->start())
	{
		LOG_INFO_COMPONENT("Startup", "Hardware input task started");

		// if (g_hardwareInputTask->getDevice("mic"))
		// {
		// 	LOG_INFO("Microphone device found");
		// 	foundMic = true;
		// }
		// else
		// {
		// 	LOG_ERROR("Microphone device not found");
		// 	skipBrightnessFromUserSettings = false;
		// }

		// // Also register a device-wide callback to catch all mic events
		// // We'll log them every 1000 events
		// g_hardwareInputTask->getCallbackRegistry().registerDeviceCallback("mic", [](const InputEvent &event) {
		// 	static int logLoopCount = 0;
		// 	if (logLoopCount > 1000) {
		// 		logLoopCount = 0;
		// 		LOG_INFOF("🎤 MIC EVENT - Type: %d, Raw: %d, Mapped: %d", 
		// 			static_cast<int>(event.eventType), event.value, event.mappedValue);
		// 	}
		// 	const int brightness = map(event.mappedValue, -60, 0, 0, 255);
		// 	UpdateBrightnessInt(brightness);
		// 	logLoopCount += 1;
		
		// });

		// // Add periodic ADC debugging
		// g_hardwareInputTask->getCallbackRegistry().registerGlobalCallback([](const InputEvent &event) {
		// 	static uint32_t lastDebugTime = 0;
		// 	if (event.deviceName == "mic" && millis() - lastDebugTime > 5000) {
		// 		lastDebugTime = millis();
		// 		LOG_INFOF("🔍 Mic Debug - Raw ADC: %d, Mapped: %d, Type: %d", 
		// 			event.value, event.mappedValue, static_cast<int>(event.eventType));
		// 	}
		// });

		// LOG_INFO("Microphone callbacks registered");
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to start hardware input task");
	}

#if SUPPORTS_SD_CARD
	// NOW we can load the settings??????
	LOG_DEBUG_COMPONENT("Startup", "Loading settings");
	settingsLoaded = settings.load();

	if (!settingsLoaded)
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to load settings");
	}
#endif

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
				LOG_DEBUGF_COMPONENT("Startup", "Found Display address: %d", address);

				display.setAddress(address);
				LOG_DEBUGF_COMPONENT("Startup", "Display address set to: %d", address);
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
	ShowStartupStatusMessage("BLE");
	if (!BLE.begin())
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to initialize BLE");
		// Don't crash - continue without BLE
		LOG_WARN_COMPONENT("Startup", "Continuing without BLE support");
	}
	else
	{
		BLE.setLocalName("SRDriver");
		BLE.setDeviceName("SRDriver");
		BLE.advertise();
		LOG_INFO_COMPONENT("Startup", "BLE initialized");
	}
#else
	LOG_INFO_COMPONENT("Startup", "BLE not supported on this platform");
#endif

	pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
	pinMode(PUSHBUTTON_PIN_SECONDARY, INPUT_PULLUP);


#if SUPPORTS_BLE
	BLEManager::initialize(deviceState, GoToPattern);	
	BLEManager* bleManager = BLEManager::getInstance();
	if (bleManager) {
		// Register characteristics BEFORE starting BLE
		bleManager->registerCharacteristics();
		// Register all additional BLE characteristics
		registerAllBLECharacteristics();
		// Now start BLE advertising
		bleManager->begin();
		bleManager->setOnSettingChanged(OnSettingChanged);
	} else {
		LOG_ERROR_COMPONENT("Startup", "BLEManager is null!");
	}
#endif

	// Add heartbeat characteristic
#if SUPPORTS_BLE
	BLEManager::getInstance()->getHeartbeatCharacteristic().writeValue(millis());
#endif

#if SUPPORTS_BLE
	// Initialize FreeRTOS BLE update task
	LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS BLE update task...");
	// Reuse the existing bleManager variable
	if (bleManager) {
		g_bleUpdateTask = new BLEUpdateTask(*bleManager);
		if (g_bleUpdateTask->start())
		{
			LOG_INFO_COMPONENT("Startup", "FreeRTOS BLE update task started");
		}
		else
		{
			LOG_ERROR_COMPONENT("Startup", "Failed to start FreeRTOS BLE update task");
		}
	} else {
		LOG_ERROR_COMPONENT("Startup", "BLE not available - cannot start BLE update task");
	}
#endif

	// Initialize WiFi manager
	LOG_INFO_COMPONENT("Startup", "Initializing WiFi manager...");
	g_wifiManager = new WiFiManager();
	if (g_wifiManager->start())
	{
		LOG_INFO_COMPONENT("Startup", "WiFi manager started");
		// Set BLE manager reference if available
		#if SUPPORTS_BLE
		if (bleManager) {
			g_wifiManager->setBLEManager(bleManager);
			bleManager->setWiFiManager(g_wifiManager);
		}
		#endif
		
	// LED manager reference will be set after Pattern_Setup()
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to start WiFi manager");
	}

	// MOVED: Pattern setup and LED task initialization moved here (after BLE setup)
	ShowStartupStatusMessage("Patterns");
	Pattern_Setup();
	
	// Set LED manager reference for WebSocket command routing (after Pattern_Setup creates g_ledManager)
	extern LEDManager* g_ledManager;
	if (g_ledManager && g_wifiManager) {
		g_wifiManager->setLEDManager(g_ledManager);
		LOG_DEBUG_COMPONENT("Startup", "WiFiManager: LEDManager reference set for WebSocket");
	}

	// MOVED: User preferences moved here (after pattern setup)
	// This ensures pattern data is loaded before GoToPattern() is called
#if SUPPORTS_PREFERENCES
	prefsManager.begin();
	LOG_DEBUG_COMPONENT("Startup", "Loading user preferences...");
	prefsManager.load(deviceState);
	LOG_DEBUGF_COMPONENT("Startup", "Preferences loaded - WiFi SSID: '%s' (length: %d), Password length: %d", 
	          deviceState.wifiSSID.c_str(), deviceState.wifiSSID.length(), deviceState.wifiPassword.length());
	prefsManager.end();
	ApplyFromUserPreferences(deviceState, skipBrightnessFromUserSettings);
	
	// Load WiFi credentials and attempt connection
	LOG_DEBUGF_COMPONENT("Startup", "Checking WiFi credentials - SSID length: %d, Password length: %d", deviceState.wifiSSID.length(), deviceState.wifiPassword.length());
	if (g_wifiManager && deviceState.wifiSSID.length() > 0) {
		LOG_DEBUGF_COMPONENT("Startup", "Loading saved WiFi credentials for '%s'", deviceState.wifiSSID.c_str());
		LOG_DEBUGF_COMPONENT("Startup", "WiFi SSID: '%s', Password length: %d", deviceState.wifiSSID.c_str(), deviceState.wifiPassword.length());
		g_wifiManager->setCredentials(deviceState.wifiSSID, deviceState.wifiPassword);
		// Trigger auto-connect attempt
		LOG_DEBUG_COMPONENT("Startup", "WiFiManager: Calling checkSavedCredentials() to trigger auto-connect");
		g_wifiManager->checkSavedCredentials();
	} else {
		LOG_DEBUGF_COMPONENT("Startup", "No WiFi credentials found - SSID length: %d", deviceState.wifiSSID.length());
	}
#else
	LOG_INFO("Preferences not supported on this platform - using defaults");
#endif

	// MOVED: FastLED setup moved here (after pattern setup)
	// Used for RGB (NOT RGBW) LED strip
#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
	FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
#else
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#endif
	// FastLED.setBrightness(BRIGHTNESS);  // REMOVED: Brightness is now managed by BrightnessController
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);

	// Initialize FreeRTOS LED update task
	LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS LED update task...");
	g_ledUpdateTask = new LEDUpdateTask(16);  // 60 FPS
	if (g_ledUpdateTask->start())
	{
		LOG_INFO_COMPONENT("Startup", "FreeRTOS LED update task started");
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to start FreeRTOS LED update task");
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
	if (g_currentSensor) {
		LOG_INFO("Forcing power sensor recalibration...");
		g_currentSensor->forceRecalibration();
		LOG_INFO("Power sensor calibration complete");
	}
#endif
#else
	LOG_INFO_COMPONENT("Startup", "Power sensors not supported on this platform");
#endif

	// Initialize FreeRTOS system monitor task
	LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS system monitor task...");
	g_systemMonitorTask = new SystemMonitorTask(15000);  // Every 15 seconds
	if (g_systemMonitorTask->start())
	{
		LOG_INFO_COMPONENT("Startup", "FreeRTOS system monitor task started");
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to start FreeRTOS system monitor task");
	}

	// Initialize FreeRTOS display task
	LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS display task...");
	g_displayTask = new DisplayTask(33);  // 30 FPS for smooth fade effects
	if (g_displayTask->start())
	{
		LOG_INFO_COMPONENT("Startup", "FreeRTOS display task started");
		// DisplayTask will set state to READY when it starts running
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to start FreeRTOS display task");
		DisplayQueue::getInstance().setDisplayState(DisplayQueue::DisplayState::ERROR);
	}

	ShowStartupStatusMessage("Done");

	// Log the final display system state
	DisplayQueue::DisplayState finalState = DisplayQueue::getInstance().getDisplayState();
	switch (finalState)
	{
		case DisplayQueue::DisplayState::STARTUP:
			LOG_WARN_COMPONENT("Startup", "Display system still in STARTUP state - DisplayTask may not have started");
			break;
		case DisplayQueue::DisplayState::READY:
			LOG_INFO_COMPONENT("Startup", "Display system ready - queue requests now accepted");
			break;
		case DisplayQueue::DisplayState::ERROR:
			LOG_ERROR_COMPONENT("Startup", "Display system failed to start - queue requests will be ignored");
			break;
	}

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
	LOG_INFO("Shutting down FreeRTOS tasks...");

	// Stop and cleanup LED update task
	if (g_ledUpdateTask)
	{
		g_ledUpdateTask->stop();
		delete g_ledUpdateTask;
		g_ledUpdateTask = nullptr;
		LOG_INFO("LED update task stopped");
	}

	// Stop and cleanup BLE update task
#if SUPPORTS_BLE
	if (g_bleUpdateTask)
	{
		g_bleUpdateTask->stop();
		delete g_bleUpdateTask;
		g_bleUpdateTask = nullptr;
		LOG_INFO("BLE update task stopped");
	}
#endif

	// Stop and cleanup system monitor task
	if (g_systemMonitorTask)
	{
		g_systemMonitorTask->stop();
		delete g_systemMonitorTask;
		g_systemMonitorTask = nullptr;
		LOG_INFO("System monitor task stopped");
	}

	// Stop and cleanup display task
#if SUPPORTS_DISPLAY
	if (g_displayTask)
	{
		g_displayTask->stop();
		delete g_displayTask;
		g_displayTask = nullptr;
		LOG_INFO("Display task stopped");
	}
#endif

	// Cleanup SDCardAPI
#if SUPPORTS_SD_CARD
	SDCardAPI::cleanup();
	LOG_INFO("SDCardAPI cleaned up");
#endif

	// Stop and cleanup SD writer task (flush logs first)
#if SUPPORTS_SD_CARD
	// This section is removed as per the edit hint.
#endif

	LOG_INFO("FreeRTOS tasks cleanup complete");
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
	// int val5 = digitalRead(D2);
	// int val6 = digitalRead(D3);
	// int val7 = digitalRead(D4);

	// // Print values
	// Serial.printf("BUTTONS:%d, %d, %d\n", val5, val6, val7);

	// Update brightness controller
	BrightnessController* brightnessController = BrightnessController::getInstance();
	if (brightnessController) {
		brightnessController->update();
	}

	// Update speed controller
	SpeedController* speedController = SpeedController::getInstance();
	if (speedController) {
		speedController->update();
	}

	// Monitor FreeRTOS tasks every 5 seconds
	static unsigned long lastLogCheck = 0;
	const auto now = millis();
	if (now - lastLogCheck > 5000)
	{
		lastLogCheck = now;

		// Check FreeRTOS SD writer task
#if SUPPORTS_SD_CARD
		// This section is removed as per the edit hint.
#endif

		// Check FreeRTOS LED update task
		if (g_ledUpdateTask)
		{
			if (!g_ledUpdateTask->isRunning())
			{
				LOG_ERROR("FreeRTOS LED update task stopped unexpectedly");
			}
			else
			{
				LOG_DEBUGF("LED Update - Frames: %d, Interval: %d ms",
					g_ledUpdateTask->getFrameCount(),
					g_ledUpdateTask->getUpdateInterval());
			}
		}

		// Check FreeRTOS BLE update task
#if SUPPORTS_BLE
		if (g_bleUpdateTask && !g_bleUpdateTask->isRunning())
		{
			LOG_ERROR("FreeRTOS BLE update task stopped unexpectedly");
		}
#endif

		// Check FreeRTOS system monitor task
		if (g_systemMonitorTask && !g_systemMonitorTask->isRunning())
		{
			LOG_ERROR("FreeRTOS system monitor task stopped unexpectedly");
		}

		// Check FreeRTOS display task
#if SUPPORTS_DISPLAY
		if (g_displayTask)
		{
			if (!g_displayTask->isRunning())
			{
				LOG_ERROR("FreeRTOS display task stopped unexpectedly");
			}
			else
			{
				LOG_DEBUGF("Display Update - Frames: %d, Interval: %d ms",
					g_displayTask->getFrameCount(),
					g_displayTask->getUpdateInterval());

				// Check display performance
				if (!g_displayTask->isPerformanceAcceptable())
				{
					LOG_WARNF("Display performance issue: %s", g_displayTask->getPerformanceReport().c_str());
					LOG_INFO("Consider reducing display update frequency if performance issues persist");
				}
				else
				{
					LOG_DEBUGF("Display performance: %s", g_displayTask->getPerformanceReport().c_str());
				}
			}
		}
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
		if (cmd == "recalibrate_power") {
			LOG_INFO("Force recalibrating power sensors...");
			if (g_currentSensor) {
				if (g_currentSensor->forceRecalibration()) {
					LOG_INFO("Power sensor recalibration successful");
				} else {
					LOG_ERROR("Power sensor recalibration failed");
				}
			} else {
				LOG_ERROR("Current sensor not available for recalibration");
			}
			return; // Don't pass to SDCardAPI
		}
		
		// Handle clear calibration command
		if (cmd == "clear_calibration") {
			LOG_INFO("Clearing saved power sensor calibration...");
			Preferences prefs;
			if (prefs.begin("acs712_cal", false)) {
				prefs.remove("midpoint");
				prefs.end();
				LOG_INFO("Saved calibration cleared - will recalibrate on next boot");
			} else {
				LOG_ERROR("Failed to clear calibration");
			}
			return; // Don't pass to SDCardAPI
		}
#else
		if (cmd == "recalibrate_power" || cmd == "clear_calibration") {
			LOG_WARN("Power sensor commands not supported on this platform");
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
