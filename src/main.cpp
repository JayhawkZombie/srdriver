#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0
// #define FASTLED_RP2040_CLOCKLESS_PIO 0

#include <FastLED.h>
#include <stdint.h>
#include "Utils.hpp"
#include "Globals.h"
#include "../lights/LightPlayer2.h"
#include "../lights/DataPlayer.h"
#include "hal/Button.hpp"
#include "hal/Potentiometer.hpp"
#include "die.hpp"
#include "../lights/WavePlayer.h"
#include <ArduinoBLE.h>

#include "GlobalState.h"
#include "DeviceState.h"
#include "BLEManager.h"
#include "PatternManager.h"
#include "UserPreferences.h"

#include <SD.h>
#include <array>
#include <memory>

// FreeRTOS includes
#include "freertos/SRTask.h"
#include "freertos/LogManager.h"
#include "freertos/SDWriterTask.h"
#include "freertos/LEDUpdateTask.h"
#include "freertos/BLEUpdateTask.h"
#include "freertos/SDCardIndexerTask.h"
#include "freertos/SystemMonitorTask.h"

#include "utility/SDUtils.h"
#include "utility/OutputManager.h"

#include "SDCardAPI.h"
#include "utility/FileParser.h"

#include "hal/SSD_1306Component.h"
#include "freertos/DisplayTask.h"

SSD1306_Display display;

// Global FreeRTOS task instances
static SDWriterTask* g_sdWriterTask = nullptr;
static LEDUpdateTask* g_ledUpdateTask = nullptr;
static BLEUpdateTask* g_bleUpdateTask = nullptr;
static SDCardIndexerTask* g_sdCardIndexerTask = nullptr;
static SystemMonitorTask* g_systemMonitorTask = nullptr;
static DisplayTask* g_displayTask = nullptr;

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
Potentiometer brightnessPot(POTENTIOMETER_PIN_BRIGHTNESS);
Potentiometer speedPot(POTENTIOMETER_PIN_SPEED);
Potentiometer extraPot(POTENTIOMETER_PIN_EXTRA);

CRGB leds[NUM_LEDS];

// LightPlayer2 LtPlay2; // Declare the LightPlayer2 instance

// storage for a 3 step pattern #100
uint8_t stateData[24];// enough for 24*8 = 192 = 3*64 state assignments

WavePlayer largeWavePlayer;
DataPlayer dataPlayer;

// Function declarations for main.cpp specific functions
void CheckPotentiometers();

// Global SD card availability flag
bool g_sdCardAvailable = false;

void wait_for_serial_connection()
{
	uint32_t timeout_end = millis() + 2000;
	Serial.begin(9600);
	while (!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
}

void OnSettingChanged(DeviceState &state)
{
	Serial.println("Device state changed");
	FastLED.setBrightness(state.brightness);
	SaveUserPreferences(state);
	// Optionally: save preferences, update UI, etc.
}

void setup()
{
	wait_for_serial_connection();
	LOG_INFO("Beginning setup");

	display.setupDisplay();

	g_sdCardAvailable = SD.begin(SDCARD_PIN);
	if (!g_sdCardAvailable)
	{
		LOG_ERROR("Failed to initialize SD card - continuing without SD card support");
	} else {
		LOG_INFO("SD card initialized successfully");
	}

	if (!BLE.begin())
	{
		LOG_ERROR("Failed to initialize BLE");
		while (1) {};
	}

	BLE.setLocalName("SRDriver");
	BLE.setDeviceName("SRDriver");
	BLE.advertise();
	LOG_INFO("BLE initialized");

	// Used for RGB (NOT RGBW) LED strip
#if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
	FastLED.addLeds(&rgbwEmu, leds, NUM_LEDS);
#else
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#endif
	FastLED.setBrightness(BRIGHTNESS);
	// Control power usage if computer is complaining/LEDs are misbehaving
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, NUM_LEDS * 20);

	Pattern_Setup();

	// Add heartbeat characteristic
	bleManager.getHeartbeatCharacteristic().writeValue(millis());

	LOG_INFO("Setup complete");
	pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
	pinMode(PUSHBUTTON_PIN_SECONDARY, INPUT_PULLUP);

	prefsManager.begin();
	prefsManager.load(deviceState);
	prefsManager.save(deviceState);
	prefsManager.end();

	ApplyFromUserPreferences(deviceState);

	bleManager.begin();
	bleManager.setOnSettingChanged(OnSettingChanged);
	
	// Initialize FreeRTOS logging system
	LOG_INFO("Initializing FreeRTOS logging system...");
	g_sdWriterTask = new SDWriterTask("/logs/srdriver.log");
	if (g_sdWriterTask->start()) {
		LOG_INFO("FreeRTOS logging system started");
		
		// Wait for task to initialize
		delay(100);
		
		// Test logging
		LOG_INFO("FreeRTOS logging system initialized");
		LOG_PRINTF("System started at: %d ms", millis());
		LOG_PRINTF("SD card available: %s", g_sdCardAvailable ? "yes" : "no");
		
	} else {
		LOG_ERROR("Failed to start FreeRTOS logging system");
	}
	
	// Initialize FreeRTOS LED update task
	LOG_INFO("Initializing FreeRTOS LED update task...");
	g_ledUpdateTask = new LEDUpdateTask(16);  // 60 FPS
	if (g_ledUpdateTask->start()) {
		LOG_INFO("FreeRTOS LED update task started");
	} else {
		LOG_ERROR("Failed to start FreeRTOS LED update task");
	}
	
	// Initialize FreeRTOS BLE update task
	LOG_INFO("Initializing FreeRTOS BLE update task...");
	g_bleUpdateTask = new BLEUpdateTask(bleManager);
	if (g_bleUpdateTask->start()) {
		LOG_INFO("FreeRTOS BLE update task started");
	} else {
		LOG_ERROR("Failed to start FreeRTOS BLE update task");
	}
	
	// Initialize FreeRTOS system monitor task
	LOG_INFO("Initializing FreeRTOS system monitor task...");
	g_systemMonitorTask = new SystemMonitorTask(15000);  // Every 15 seconds
	if (g_systemMonitorTask->start()) {
		LOG_INFO("FreeRTOS system monitor task started");
	} else {
		LOG_ERROR("Failed to start FreeRTOS system monitor task");
	}
	
	// Initialize FreeRTOS SD card indexer task
	LOG_INFO("Initializing FreeRTOS SD card indexer task...");
	g_sdCardIndexerTask = new SDCardIndexerTask(1);  // 1ms intervals for fast indexing
	if (g_sdCardIndexerTask->start()) {
		LOG_INFO("FreeRTOS SD card indexer task started");
	} else {
		LOG_ERROR("Failed to start FreeRTOS SD card indexer task");
	}
	
	// Initialize FreeRTOS display task
	LOG_INFO("Initializing FreeRTOS display task...");
	g_displayTask = new DisplayTask(200);  // 5 FPS for display updates
	if (g_displayTask->start()) {
		LOG_INFO("FreeRTOS display task started");
	} else {
		LOG_ERROR("Failed to start FreeRTOS display task");
	}
	
	// Initialize SDCardAPI singleton
	SDCardAPI::initialize();
	
	// Initialize SD card systems if available
	if (g_sdCardAvailable) {
		// Initialize SD card systems
		g_sdCardIndexerTask->begin("/", 2); // Start indexing SD card at setup
		LOG_INFO("SRDriver starting up with SD card support");
	} else {
		LOG_INFO("SRDriver starting up (no SD card - logging to serial)");
	}
	
	LOG_INFO("Device monitoring handled by FreeRTOS SystemMonitorTask");
}

/**
 * Clean up FreeRTOS tasks
 * Call this during shutdown or restart
 */
void cleanupFreeRTOSTasks() {
	LOG_INFO("Shutting down FreeRTOS tasks...");
	
	// Stop and cleanup LED update task
	if (g_ledUpdateTask) {
		g_ledUpdateTask->stop();
		delete g_ledUpdateTask;
		g_ledUpdateTask = nullptr;
		LOG_INFO("LED update task stopped");
	}
	
	// Stop and cleanup BLE update task
	if (g_bleUpdateTask) {
		g_bleUpdateTask->stop();
		delete g_bleUpdateTask;
		g_bleUpdateTask = nullptr;
		LOG_INFO("BLE update task stopped");
	}
	
	// Stop and cleanup system monitor task
	if (g_systemMonitorTask) {
		g_systemMonitorTask->stop();
		delete g_systemMonitorTask;
		g_systemMonitorTask = nullptr;
		LOG_INFO("System monitor task stopped");
	}
	
	// Stop and cleanup SD card indexer task
	if (g_sdCardIndexerTask) {
		g_sdCardIndexerTask->stop();
		delete g_sdCardIndexerTask;
		g_sdCardIndexerTask = nullptr;
		LOG_INFO("SD card indexer task stopped");
	}
	
	// Stop and cleanup display task
	if (g_displayTask) {
		g_displayTask->stop();
		delete g_displayTask;
		g_displayTask = nullptr;
		LOG_INFO("Display task stopped");
	}
	
	// Cleanup SDCardAPI
	SDCardAPI::cleanup();
	LOG_INFO("SDCardAPI cleaned up");
	
	// Stop and cleanup SD writer task (flush logs first)
	if (g_sdWriterTask) {
		g_sdWriterTask->forceFlush();
		g_sdWriterTask->stop();
		delete g_sdWriterTask;
		g_sdWriterTask = nullptr;
		LOG_INFO("SD writer task stopped");
	}
	
	LOG_INFO("FreeRTOS tasks cleanup complete");
}

void DrawError(const CRGB &color)
{
	for (int i = 0; i < LEDS_MATRIX_X; i += 2)
	{
		leds[i] = color;
	}
}

int sharedCurrentIndexState = 0;
float speedMultiplier = 8.0f;

void CheckPotentiometers()
{
	// Always check brightness potentiometer regardless of potensControlColor
	// Call getValue() to update the change detection state
	brightnessPot.getValue();

	if (brightnessPot.hasChanged())
	{
		LOG_INFO("Brightness potentiometer has changed");
		float brightness = brightnessPot.getCurveMappedValue();
		UpdateBrightness(brightness);
		bleManager.updateBrightness();
		brightnessPot.resetChanged();
	}

	int speed = speedPot.getMappedValue(0, 255);
	int extra = extraPot.getMappedValue(0, 255);
	speedMultiplier = speed / 255.f * 20.f;
}

void loop()
{
	// Optionally, add a small delay if needed
	delay(1);

	// Monitor FreeRTOS tasks every 5 seconds
	static unsigned long lastLogCheck = 0;
	const auto now = millis();
	if (now - lastLogCheck > 5000) {
		lastLogCheck = now;
		
		// Check FreeRTOS SD writer task
		if (g_sdWriterTask) {
			LOG_DEBUGF("FreeRTOS Log Queue - Items: %d, Available: %d", 
					  g_sdWriterTask->getLogQueue()->getItemCount(),
					  g_sdWriterTask->getLogQueue()->getSpacesAvailable());
			
			if (!g_sdWriterTask->isRunning()) {
				LOG_ERROR("FreeRTOS SD writer task stopped unexpectedly");
			}
		}
		
		// Check FreeRTOS LED update task
		if (g_ledUpdateTask) {
			if (!g_ledUpdateTask->isRunning()) {
				LOG_ERROR("FreeRTOS LED update task stopped unexpectedly");
			} else {
				LOG_DEBUGF("LED Update - Frames: %d, Interval: %d ms", 
						  g_ledUpdateTask->getFrameCount(), 
						  g_ledUpdateTask->getUpdateInterval());
			}
		}
		
		// Check FreeRTOS BLE update task
		if (g_bleUpdateTask && !g_bleUpdateTask->isRunning()) {
			LOG_ERROR("FreeRTOS BLE update task stopped unexpectedly");
		}
		
		// Check FreeRTOS system monitor task
		if (g_systemMonitorTask && !g_systemMonitorTask->isRunning()) {
			LOG_ERROR("FreeRTOS system monitor task stopped unexpectedly");
		}
		
		// Check FreeRTOS display task
		if (g_displayTask) {
			if (!g_displayTask->isRunning()) {
				LOG_ERROR("FreeRTOS display task stopped unexpectedly");
			} else {
				LOG_DEBUGF("Display Update - Frames: %d, Interval: %d ms", 
						  g_displayTask->getFrameCount(), 
						  g_displayTask->getUpdateInterval());
			}
		}
		
		// Log detailed task information every 30 seconds
		static unsigned long lastDetailedCheck = 0;
		if (now - lastDetailedCheck > 30000) {
			lastDetailedCheck = now;
			if (g_systemMonitorTask) {
				g_systemMonitorTask->logDetailedTaskInfo();
				
				// Add power efficiency monitoring
				uint8_t powerScore = g_systemMonitorTask->getPowerEfficiencyScore();
				LOG_PRINTF("Power Efficiency Score: %d/100", powerScore);
				
				if (powerScore < 70) {
					LOG_WARN("Low power efficiency detected - consider optimizations");
					g_systemMonitorTask->suggestPowerOptimizations();
				}
			}
		}
	}

	// Serial command to trigger file streaming
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
		cmd.trim();
        
        // Log the command using new FreeRTOS logging
        LOG_INFO("Serial command received: " + cmd);
        
        // Set output target to SERIAL_OUTPUT for commands received via serial
        SDCardAPI::getInstance().setOutputTarget(OutputTarget::SERIAL_OUTPUT);
        SDCardAPI::getInstance().handleCommand(cmd);
        // Reset to BLE for future BLE commands
        SDCardAPI::getInstance().setOutputTarget(OutputTarget::BLE);
    }
}
