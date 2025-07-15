// #define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0
// #define FASTLED_RP2040_CLOCKLESS_PIO 0

#include <FastLED.h>
#include <stdint.h>
#include "Utils.hpp"
#include "Globals.h"
#include "LightPlayer2.h"
#include "DataPlayer.h"
#include "hal/Button.hpp"
#include "hal/Potentiometer.hpp"
#include "die.hpp"
#include "WavePlayer.h"
#include <ArduinoBLE.h>

#include "GlobalState.h"
#include "DeviceState.h"
#include "BLEManager.h"
#include "PatternManager.h"
#include "UserPreferences.h"

#include <SD.h>
#include <TaskScheduler.h>
#include <array>
#include <memory>

// FreeRTOS includes
#include "freertos/SRTask.h"
#include "freertos/LogManager.h"
#include "freertos/SDWriterTask.h"
#include "freertos/LEDUpdateTask.h"
#include "freertos/ExampleTasks.h"

// #include "tasks/LEDUpdateTask.h"  // Removed - using FreeRTOS LEDUpdateTask instead
#include "tasks/BLEUpdateTask.h"
#include "tasks/FileStreamer.h"
#include "tasks/SDCardIndexer.h"
// #include "tasks/LogWriterTask.h"  // Removed - using FreeRTOS logging instead
#include "tasks/DeviceMonitor.h"

#include "utility/SDUtils.h"
#include "utility/OutputManager.h"
// #include "utility/LogManager.h"  // Removed - using FreeRTOS LogManager instead

#include "SDCardAPI.h"
#include "utility/FileParser.h"

// Global FreeRTOS task instances
static SDWriterTask* g_sdWriterTask = nullptr;
static LEDUpdateTask* g_ledUpdateTask = nullptr;
static SystemMonitorTask* g_systemMonitorTask = nullptr;

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
void EnterSettingsMode();
void MoveToNextSetting();
void ExitSettingsMode();
void UpdateLEDsForSettings(int potentiometerValue);
void CheckPotentiometers();
void HandleBLE();

// Heartbeat timing
unsigned long lastHeartbeatSent = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 5000;

// Scheduler instance
Scheduler runner;

// Create task instances
// LEDUpdateTask ledUpdateTask;  // Removed - using FreeRTOS LEDUpdateTask instead
// Task ledTask(33, TASK_FOREVER, [](){ ledUpdateTask.update(); }, &runner, false); // Disabled - using FreeRTOS task instead

BLEUpdateTask bleUpdateTask(bleManager);
Task bleTask(10, TASK_FOREVER, [](){ bleUpdateTask.update(); }, &runner, true);

FileStreamer fileStreamer;
Task fileStreamTask(5, TASK_FOREVER, [](){
    fileStreamer.update();
    if (!fileStreamer.isActive()) fileStreamTask.disable();
}, &runner, false); // Start disabled

SDCardIndexer sdCardIndexer;
Task sdCardIndexTask(1, TASK_FOREVER, [](){
    sdCardIndexer.update();
    if (!sdCardIndexer.isActive()) sdCardIndexTask.disable();
}, &runner, true); // Start enabled

// #include "tasks/LogWriterTask.h"  // Removed - using FreeRTOS logging instead
DeviceMonitor deviceMonitor;
Task deviceMonitorTask(1000, TASK_FOREVER, [](){ 
    deviceMonitor.update(); 
}, &runner, true); // Run every second, always enabled

// Create a callback function to enable the file stream task
auto enableFileStreamTask = []() { fileStreamTask.enable(); };

// Task classes - all in main.cpp where they work

SDCardAPI sdCardAPI(fileStreamer, sdCardIndexer, enableFileStreamTask);

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

	g_sdCardAvailable = SD.begin(SDCARD_PIN);
	if (!g_sdCardAvailable)
	{
		Serial.println("Failed to initialize SD card - continuing without SD card support");
	} else {
		Serial.println("SD card initialized successfully");
	}

	// Test FileParser with /dads.txt only if SD card is available
	if (g_sdCardAvailable) {
		Serial.println("[Test] Reading from /dads.txt using FileParser...");
		FileParser input("/dads.txt", FileParser::Mode::READ);
		if (input.isOpen()) {
			int x;
			float y;
			String message;
			int z;
			input >> x >> y >> message >> z;
			Serial.print("[Test] x: "); Serial.println(x);
			Serial.print("[Test] y: "); Serial.println(y);
			Serial.print("[Test] message: "); Serial.println(message);
			Serial.print("[Test] z: "); Serial.println(z);
			input.close();
		} else {
			Serial.println("[Test] Failed to open /dads.txt");
		}

		listDir(SD, "/", 0);
	}

	if (!BLE.begin())
	{
		Serial.println("Failed to initialize BLE");
		while (1) {};
	}

	BLE.setLocalName("SRDriver");
	BLE.setDeviceName("SRDriver");
	// Serial.print("BLE local name: ");
	// Serial.println(BLE.localName());

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
	Serial.println("Setup");

	Pattern_Setup();

	// Add heartbeat characteristic
	bleManager.getHeartbeatCharacteristic().writeValue(millis());

	Serial.println("Setup complete");
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
	Serial.println("[Main] Initializing FreeRTOS logging system...");
	g_sdWriterTask = new SDWriterTask("/logs/srdriver.log");
	if (g_sdWriterTask->start()) {
		Serial.println("[Main] FreeRTOS logging system started");
		
		// Wait for task to initialize
		delay(100);
		
		// Test logging
		LOG_INFO("FreeRTOS logging system initialized");
		LOG_PRINTF("System started at: %d ms", millis());
		LOG_PRINTF("SD card available: %s", g_sdCardAvailable ? "yes" : "no");
		
	} else {
		Serial.println("[Main] Failed to start FreeRTOS logging system");
	}
	
	// Initialize FreeRTOS LED update task
	Serial.println("[Main] Initializing FreeRTOS LED update task...");
	g_ledUpdateTask = new LEDUpdateTask(16);  // 60 FPS
	if (g_ledUpdateTask->start()) {
		Serial.println("[Main] FreeRTOS LED update task started");
		LOG_INFO("FreeRTOS LED update task started");
	} else {
		Serial.println("[Main] Failed to start FreeRTOS LED update task");
		LOG_ERROR("Failed to start FreeRTOS LED update task");
	}
	
	// Initialize FreeRTOS system monitor task
	Serial.println("[Main] Initializing FreeRTOS system monitor task...");
	g_systemMonitorTask = new SystemMonitorTask(15000);  // Every 15 seconds
	if (g_systemMonitorTask->start()) {
		Serial.println("[Main] FreeRTOS system monitor task started");
		LOG_INFO("FreeRTOS system monitor task started");
	} else {
		Serial.println("[Main] Failed to start FreeRTOS system monitor task");
		LOG_ERROR("Failed to start FreeRTOS system monitor task");
	}
	
	// Initialize SD card systems if available
	if (g_sdCardAvailable) {
		// Initialize SD card systems
		sdCardIndexer.begin("/", 2); // Start indexing SD card at setup
		
		LOG_INFO("SRDriver starting up with SD card support");
		Serial.println("[Main] SD card systems initialized");
	} else {
		LOG_INFO("SRDriver starting up (no SD card - logging to serial)");
		Serial.println("[Main] SD card not available");
	}
	
	// Initialize device monitoring (works with or without SD card)
	deviceMonitor.begin();
	deviceMonitor.setInterval(30000); // Monitor every 30 seconds
	Serial.println("[Main] Device monitor initialized");
	
	runner.startNow();
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
	
	// Stop and cleanup system monitor task
	if (g_systemMonitorTask) {
		g_systemMonitorTask->stop();
		delete g_systemMonitorTask;
		g_systemMonitorTask = nullptr;
		LOG_INFO("System monitor task stopped");
	}
	
	// Stop and cleanup SD writer task (flush logs first)
	if (g_sdWriterTask) {
		g_sdWriterTask->forceFlush();
		g_sdWriterTask->stop();
		delete g_sdWriterTask;
		g_sdWriterTask = nullptr;
		LOG_INFO("SD writer task stopped");
	}
	
	Serial.println("[Main] FreeRTOS tasks cleanup complete");
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

bool hasListedFiles = false;
void listFiles()
{
	if (hasListedFiles)
		{
		return;
	}
	for (size_t i = 0; i < sdCardIndexer.getFileCount(); i++)
	{
		if (sdCardIndexer.getFile(i).isDir)
		{
			Serial.println("DIR");
		}
		else
		{
			Serial.println("FILE");
		}
		Serial.println(sdCardIndexer.getFile(i).path);
	}
	hasListedFiles = true;
}

void loop()
{
	if (g_sdCardAvailable && sdCardIndexer.isFinished())
	{
		listFiles();
	}
	const auto now = millis();
	runner.execute();
	sdCardAPI.update();
	// const auto runnerExecutionTime = now - last_ms;
	last_ms = now;
	// Optionally, add a small delay if needed
	delay(1);

	// Monitor FreeRTOS tasks every 5 seconds
	static unsigned long lastLogCheck = 0;
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
		
		// Check FreeRTOS system monitor task
		if (g_systemMonitorTask && !g_systemMonitorTask->isRunning()) {
			LOG_ERROR("FreeRTOS system monitor task stopped unexpectedly");
		}
		
		// Legacy logging system monitoring removed - using FreeRTOS logging instead
	}

	// Serial command to trigger file streaming
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
		cmd.trim();
		Serial.println("Received command: " + cmd);
        
        // Log the command using new FreeRTOS logging
        LOG_INFO("Serial command received: " + cmd);
        
        // Set output target to SERIAL_OUTPUT for commands received via serial
        sdCardAPI.setOutputTarget(OutputTarget::SERIAL_OUTPUT);
        sdCardAPI.handleCommand(cmd);
        // Reset to BLE for future BLE commands
        sdCardAPI.setOutputTarget(OutputTarget::BLE);
    }
}
