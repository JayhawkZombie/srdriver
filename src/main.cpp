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

#include "tasks/LEDUpdateTask.h"
#include "tasks/BLEUpdateTask.h"
#include "tasks/FileStreamer.h"
#include "tasks/SDCardIndexer.h"

#include "utility/SDUtils.h"

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
LEDUpdateTask ledUpdateTask;
Task ledTask(33, TASK_FOREVER, [](){ ledUpdateTask.update(); }, &runner, true);

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

// Create a callback function to enable the file stream task
auto enableFileStreamTask = []() { fileStreamTask.enable(); };

// Task classes - all in main.cpp where they work
class SDCardAPI {
public:
    // Callback function type for enabling tasks
    typedef void (*TaskEnableCallback)();
    
    SDCardAPI(FileStreamer& streamer, SDCardIndexer& indexer, TaskEnableCallback enableCallback = nullptr)
        : fileStreamer(streamer), sdIndexer(indexer), enableCallback(enableCallback), busy(false) {
    }
    
    void handleCommand(const String& command) {
        String input = command;
        input.trim();

        // Find first space or colon
        int splitPos = input.indexOf(' ');
        int colonPos = input.indexOf(':');
        if (colonPos >= 0 && (splitPos < 0 || colonPos < splitPos)) {
            splitPos = colonPos;
        }

        String cmd = (splitPos >= 0) ? input.substring(0, splitPos) : input;
        String args = (splitPos >= 0) ? input.substring(splitPos + 1) : "";

        cmd.trim();
        args.trim();
        cmd.toUpperCase();

        Serial.println("Parsed command: '" + cmd + "'");
        Serial.println("Parsed args: '" + args + "'");

        if (cmd == "PRINT") {
            printFile(args);
        } else if (cmd == "LIST") {
            listFiles();
        } else if (cmd == "DELETE") {
            deleteFile(args);
        } else if (cmd == "WRITE" || cmd == "APPEND") {
            // For WRITE/APPEND, expect: WRITE filename:content
            int argColon = args.indexOf(':');
            if (argColon >= 0) {
                String filename = args.substring(0, argColon);
                String content = args.substring(argColon + 1);
                filename.trim();
                content.trim();
                if (cmd == "WRITE") writeFile(filename, content);
                else appendFile(filename, content);
            } else {
                setError(cmd + " command requires filename and content separated by colon");
            }
        } else if (cmd == "INFO") {
            getFileInfo(args);
        } else {
            setError("Unknown command: '" + cmd + "'");
        }
    }
    
    void printFile(const String& filename) {
        if (busy) {
            setError("Another operation is in progress");
            return;
        }
        
        if (fileStreamer.begin(filename.c_str())) {
            if (enableCallback) {
                enableCallback();
            }
            busy = true;
            setResult("Started printing: " + filename);
        } else {
            setError("File not found: " + filename);
        }
    }
    
    void listFiles() {
        Serial.println("----- FILE LISTING BEGIN -----");
        listDir(SD, "/", 0);
        Serial.println("----- FILE LISTING END -----");
        setResult("File listing completed - check serial output above");
    }
    
    void deleteFile(const String& filename) {
        if (SD.remove(filename.c_str())) {
            setResult("Deleted: " + filename);
        } else {
            setError("Failed to delete: " + filename);
        }
    }
    
    void writeFile(const String& filename, const String& content) {
        File file = SD.open(filename.c_str(), FILE_WRITE);
        if (file) {
            file.print(content);
            file.close();
            setResult("Written: " + filename);
        } else {
            setError("Failed to write: " + filename);
        }
    }
    
    void appendFile(const String& filename, const String& content) {
        File file = SD.open(filename.c_str(), FILE_APPEND);
        if (file) {
            file.print(content);
            file.close();
            setResult("Appended: " + filename);
        } else {
            setError("Failed to append: " + filename);
        }
    }
    
    void getFileInfo(const String& filename) {
        File file = SD.open(filename.c_str());
        if (file) {
            String info = "File: " + filename + ", Size: " + String(file.size()) + " bytes";
            file.close();
            setResult(info);
        } else {
            setError("File not found: " + filename);
        }
    }
    
    bool isBusy() const {
        return busy || fileStreamer.isActive();
    }
    
    String getLastResult() const {
        return lastResult;
    }
    
    void update() {
        // Handle completion of print operations
        if (busy && !fileStreamer.isActive() && fileStreamer.getBuffer()) {
            Serial.print("----- FILE CONTENTS BEGIN -----");
            Serial.write(fileStreamer.getBuffer(), fileStreamer.getBufferSize());
            Serial.println("\n----- FILE CONTENTS END -----");
            busy = false;
        }
    }
    
private:
    FileStreamer& fileStreamer;
    SDCardIndexer& sdIndexer;
    TaskEnableCallback enableCallback;
    
    String lastResult;
    bool busy;
    
    void setResult(const String& result) {
        lastResult = result;
        Serial.println("API Result: " + result);
    }
    
    void setError(const String& error) {
        lastResult = "ERROR: " + error;
        Serial.println("API Error: " + error);
    }
};

SDCardAPI sdCardAPI(fileStreamer, sdCardIndexer, enableFileStreamTask);

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

	if (!SD.begin(SDCARD_PIN))
	{
		Serial.println("Failed to initialize SD card");
		// Not spinning, just don't do anything
	}

	listDir(SD, "/", 0);

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
	sdCardIndexer.begin("/", 2); // Start indexing SD card at setup
	runner.startNow();
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
		Serial.println("Brightness potentiometer has changed");
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
	if (sdCardIndexer.isFinished())
	{
		listFiles();
	}
	const auto now = millis();
	runner.execute();
	sdCardAPI.update();
	// const auto runnerExecutionTime = now - last_ms;
	last_ms = now;
	// Serial.printf("Runner execution time: %dms\n", runnerExecutionTime);
	// Optionally, add a small delay if needed
	delay(1);

	// Serial command to trigger file streaming
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
		cmd.trim();
		Serial.println("Received command: " + cmd);
        sdCardAPI.handleCommand(cmd);
    }
}
