#define FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED 0
// #define FASTLED_RP2040_CLOCKLESS_PIO 0

#include <stdint.h>
#include "Utils.hpp"
#include "PlatformConfig.h"

#if SUPPORTS_LEDS
#include "Globals.h"
#include "freertos/LEDStorage.h"
#endif

#include "hal/input/buttons/Button.hpp"
#include "hal/input/potentiometers/Potentiometer.hpp"
#include "die.hpp"

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
#if SUPPORTS_LEDS
#include "freertos/LEDUpdateTask.h"
#endif
#include "freertos/BLEUpdateTask.h"
#include "freertos/WiFiManager.h"
#include "freertos/SystemMonitorTask.h"
#include "freertos/TaskManager.h"
#if PLATFORM_CROW_PANEL
#include "freertos/LVGLDisplayTask.h"
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <esp_heap_caps.h>

// LGFX display class for CrowPanel
class LGFX : public lgfx::LGFX_Device {
public:
    lgfx::Bus_RGB     _bus_instance;
    lgfx::Panel_RGB   _panel_instance;
    
    LGFX() {
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;
            cfg.pin_d0 = GPIO_NUM_15; cfg.pin_d1 = GPIO_NUM_7; cfg.pin_d2 = GPIO_NUM_6;
            cfg.pin_d3 = GPIO_NUM_5; cfg.pin_d4 = GPIO_NUM_4; cfg.pin_d5 = GPIO_NUM_9;
            cfg.pin_d6 = GPIO_NUM_46; cfg.pin_d7 = GPIO_NUM_3; cfg.pin_d8 = GPIO_NUM_8;
            cfg.pin_d9 = GPIO_NUM_16; cfg.pin_d10 = GPIO_NUM_1; cfg.pin_d11 = GPIO_NUM_14;
            cfg.pin_d12 = GPIO_NUM_21; cfg.pin_d13 = GPIO_NUM_47; cfg.pin_d14 = GPIO_NUM_48;
            cfg.pin_d15 = GPIO_NUM_45;
            cfg.pin_henable = GPIO_NUM_41; cfg.pin_vsync = GPIO_NUM_40;
            cfg.pin_hsync = GPIO_NUM_39; cfg.pin_pclk = GPIO_NUM_0;
            cfg.freq_write = 15000000;
            cfg.hsync_polarity = 0; cfg.hsync_front_porch = 40;
            cfg.hsync_pulse_width = 48; cfg.hsync_back_porch = 40;
            cfg.vsync_polarity = 0; cfg.vsync_front_porch = 1;
            cfg.vsync_pulse_width = 31; cfg.vsync_back_porch = 13;
            cfg.pclk_active_neg = 1; cfg.de_idle_high = 0; cfg.pclk_idle_high = 0;
            _bus_instance.config(cfg);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width = 800; cfg.memory_height = 480;
            cfg.panel_width = 800; cfg.panel_height = 480;
            cfg.offset_x = 0; cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }
        _panel_instance.setBus(&_bus_instance);
        _panel_instance.setBrightness(255);
        setPanel(&_panel_instance);
    }
};

// Global display instance
LGFX lcd;
hw_timer_t *lvgl_tick_timer = nullptr;
void IRAM_ATTR lv_tick_cb() { lv_tick_inc(1); }

// LVGL display variables
uint32_t screenWidth = 800;
uint32_t screenHeight = 480;
const uint32_t bufferLines = 80;
lv_disp_draw_buf_t draw_buf;
lv_color_t *buf1 = nullptr;
lv_color_t *buf2 = nullptr;
lv_disp_drv_t disp_drv;

// LVGL UI objects
lv_obj_t* screen = nullptr;
lv_obj_t* uptimeLabel = nullptr;
lv_obj_t* heapLabel = nullptr;

// Display flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    static int flushCount = 0;
    flushCount++;
    if (flushCount % 100 == 0) {  // Log every 100th flush
        Serial.printf("[LVGL] Flush #%d: area (%d,%d) to (%d,%d), size %dx%d\n",
                     flushCount, area->x1, area->y1, area->x2, area->y2,
                     area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
    }
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    lcd.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t *) &color_p->full);
    lv_disp_flush_ready(disp);
}

// Touchpad read callback (dummy)
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    data->state = LV_INDEV_STATE_REL;
}

// Initialize LVGL display
void initLVGLDisplay() {
    Serial.println("[LVGL] Allocating display buffers...");
    // Allocate buffers
    size_t buf_pixels = static_cast<size_t>(screenWidth) * bufferLines;
    Serial.printf("[LVGL] Attempting to allocate %d pixels per buffer (%d bytes each)\n", 
                  buf_pixels, buf_pixels * sizeof(lv_color_t));
    
    buf1 = (lv_color_t *) heap_caps_malloc(buf_pixels * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    buf2 = (lv_color_t *) heap_caps_malloc(buf_pixels * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    
    Serial.printf("[LVGL] Buffer allocation: buf1=%p, buf2=%p\n", buf1, buf2);
    
    if (!buf1 || !buf2) {
        Serial.println("[LVGL] PSRAM allocation failed, trying internal RAM...");
        buf_pixels = static_cast<size_t>(screenWidth) * 40;
        buf1 = (lv_color_t *) heap_caps_malloc(buf_pixels * sizeof(lv_color_t),
            MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        buf2 = nullptr;
        Serial.printf("[LVGL] Fallback buffer: buf1=%p, buf2=null\n", buf1);
    }
    
    if (!buf1) {
        Serial.println("[LVGL] ERROR: Failed to allocate display buffers!");
        return;
    }
    
    Serial.printf("[LVGL] Initializing display buffer with %d pixels\n", buf_pixels);
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_pixels);
    
    Serial.println("[LVGL] Initializing display driver...");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_t* disp = lv_disp_drv_register(&disp_drv);
    Serial.printf("[LVGL] Display driver registered: %p\n", disp);
    
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    Serial.println("[LVGL] Input device registered");
}

// Create simple UI
void createLVGLUI() {
    Serial.println("[LVGL] Creating UI...");
    screen = lv_obj_create(nullptr);
    if (!screen) {
        Serial.println("[LVGL] ERROR: Failed to create screen!");
        return;
    }
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_scr_load(screen);
    Serial.println("[LVGL] Screen created and loaded");
    
    uptimeLabel = lv_label_create(screen);
    if (uptimeLabel) {
        lv_obj_set_style_text_color(uptimeLabel, lv_color_black(), 0);
        lv_obj_align(uptimeLabel, LV_ALIGN_TOP_MID, 0, 20);
        lv_label_set_text(uptimeLabel, "Uptime: 0d 0h 0m 0s");
        lv_obj_invalidate(uptimeLabel);
        Serial.println("[LVGL] Uptime label created");
    }
    
    heapLabel = lv_label_create(screen);
    if (heapLabel) {
        lv_obj_set_style_text_color(heapLabel, lv_color_black(), 0);
        lv_obj_align(heapLabel, LV_ALIGN_TOP_MID, 0, 80);
        lv_label_set_text(heapLabel, "Heap: 0%");
        lv_obj_invalidate(heapLabel);
        Serial.println("[LVGL] Heap label created");
    }
    
    // Force invalidate entire screen
    lv_obj_invalidate(screen);
    Serial.println("[LVGL] UI creation complete");
}
#endif
#if SUPPORTS_SD_CARD
#include "utility/SDUtils.h"
#include "utility/OutputManager.h"
#include "hal/SDCardAPI.h"
#include "hal/FileParser.h"
#endif

#if SUPPORTS_DISPLAY
#include "freertos/OLEDDisplayTask.h"
#endif
#include "config/JsonSettings.h"
#include "utility/StringUtils.h"


#include "freertos/HardwareInputTask.h"
#include "hal/input/InputEvent.h"
#include "lights/LEDManager.h"

// Global FreeRTOS task instances
#if SUPPORTS_BLE
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


// LED arrays moved to freertos/LEDStorage.cpp

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
#if SUPPORTS_LEDS
	FastLED.setBrightness(state.brightness);
#endif
	SaveUserPreferences(state);
	// Optionally: save preferences, update UI, etc.
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
	if (!brightnessController)
	{
		return;
	}

	int brightness = brightnessController->getBrightness();
	if (didChangeUp)
	{
		encoderBrightness++;
		encoderBrightness = constrain(encoderBrightness, 0, 255);
		brightnessController->updateBrightness(encoderBrightness);
	}
	else
	{
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
	if (currentEffectIndex >= effectOrderJsonStrings.size())
	{
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
	}
	else if (rotEncButton.pollEvent() == -1)
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
#if SUPPORTS_LEDS
	// Many computers will complain about power consumption, and sometimes
	// will just outright disable the USB device, so we'll attempt to limit
	// power if we can detect a USB serial connection (not the best check,
	// since it'll miss most cases, but it's better than nothing)
	if (Serial)
	{
		FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
	}
#endif
}

void OnShutdown()
{
	LOG_INFO_COMPONENT("Main", "OnShutdown called");
	isShuttingDown = true;
#if SUPPORTS_LEDS
	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i] = CRGB::Black;
	}
	FastLED.setBrightness(0);
	FastLED.clear();
	FastLED.show();
	digitalWrite(LED_PIN, LOW);
#endif
	// esp_restart();
}

void setup()
{
	// Serial.begin(9600);
	Serial.begin(115200);
	// wait_for_serial();
	
#if SUPPORTS_LEDS
	// Initialize LEDs early (black them out)
	LEDUpdateTask::initializeLEDs();
#else
	// LEDs not supported on this platform
	LOG_INFO_COMPONENT("Startup", "LED support disabled for this platform");
#endif

	esp_register_shutdown_handler(OnShutdown);

	SerialAwarePowerLimiting();
	// SetupOthers();
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


	// Initialize FreeRTOS logging system
	// LOG_INFO_COMPONENT("Startup", "Initializing FreeRTOS logging system...");
#if SUPPORTS_SD_CARD
	LogManager::getInstance().initialize();

	// Configure log filtering (optional - can be enabled/disabled)
	// Uncomment the line below to show only WiFiManager logs:
	std::vector<String> logFilters = { "Main", "Startup", "WebSocketServer", "WiFiManager", "LVGLDisplay", "SystemMonitor"	};
	// LOG_SET_COMPONENT_FILTER(logFilters);

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

	if (!settingsLoaded)
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to load settings");
	}
#endif

	auto &taskMgr = TaskManager::getInstance();


#if SUPPORTS_BLE
	BLEManager *bleManager = nullptr;
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
			if (bleManager)
			{
				
				taskMgr.createBLETask(*bleManager);
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
	if (taskMgr.createWiFiManager(10))
	{
		LOG_INFO_COMPONENT("Startup", "WiFi manager started");
		// Set BLE manager reference if available
#if SUPPORTS_BLE
		if (bleManager)
		{
			if (auto* wifiMgr = taskMgr.getWiFiManager()) {
				wifiMgr->setBLEManager(bleManager);
				bleManager->setWiFiManager(wifiMgr);
			}
		}
#endif
	}
	else
	{
		LOG_ERROR_COMPONENT("Startup", "Failed to start WiFi manager");
	}

#if SUPPORTS_LEDS
	Pattern_Setup();
#endif

	extern LEDManager *g_ledManager;
	if (g_ledManager) {
		if (auto* wifiMgr = taskMgr.getWiFiManager()) {
			wifiMgr->setLEDManager(g_ledManager);

		if (settingsLoaded)
		{
			// Try to load panel configs from settings, we should have an array of them under "panels"
			if (settings._doc.containsKey("panels"))
			{
				bool usePanels = false;
				JsonObject panelsObj = settings._doc["panels"];
				if (panelsObj.containsKey("usePanels"))
				{
					usePanels = panelsObj["usePanels"].as<bool>();
				}
				std::vector<PanelConfig> panelConfigs;
				JsonArray panels = panelsObj["panelConfigs"];
				for (JsonVariant panel : panels)
				{
					PanelConfig panelConfig;
					panelConfig.rows = panel["rows"].as<int>();
					panelConfig.cols = panel["cols"].as<int>();
					panelConfig.row0 = panel["row0"].as<int>();
					panelConfig.col0 = panel["col0"].as<int>();
					panelConfig.type = panel["type"].as<int>();
					panelConfig.rotIdx = panel["rotIdx"].as<int>();
					panelConfig.swapTgtRCs = panel["swapTgtRCs"].as<bool>();
					panelConfigs.push_back(panelConfig);
				}
				if (usePanels)
				{
					g_ledManager->initPanels(panelConfigs);
				}
				// LOG_DEBUGF_COMPONENT("Startup", "Loaded panel configs: %d", panelConfigs.size());
			}

		}

			LOG_DEBUG_COMPONENT("Startup", "WiFiManager: LEDManager reference set for WebSocket");
		}
	}


#if SUPPORTS_PREFERENCES
	prefsManager.begin();
	prefsManager.load(deviceState);
	prefsManager.end();
	ApplyFromUserPreferences(deviceState, skipBrightnessFromUserSettings);
	encoderBrightness = deviceState.brightness;
	// Load WiFi credentials and attempt connection

	if (settingsLoaded) {
		if (auto* wifiMgr = taskMgr.getWiFiManager()) {
			std::vector<NetworkCredentials> knownNetworksList;
			if (settings._doc.containsKey("wifi"))
			{
				JsonObject wifiObj = settings._doc["wifi"];
				if (wifiObj.containsKey("knownNetworks"))
				{
					JsonArray knownNetworks = wifiObj["knownNetworks"];
					for (JsonVariant knownNetwork : knownNetworks)
					{
						NetworkCredentials networkCredentials;
						networkCredentials.ssid = knownNetwork["ssid"].as<String>();
						networkCredentials.password = knownNetwork["password"].as<String>();
						knownNetworksList.push_back(networkCredentials);
					}
				}
			}
			LOG_DEBUGF_COMPONENT("Startup", "Known networks loaded: %d", knownNetworksList.size());
			wifiMgr->setKnownNetworks(knownNetworksList);
			if (deviceState.wifiSSID.length() > 0) {
				LOG_DEBUGF_COMPONENT("Startup", "Setting credentials for '%s'", deviceState.wifiSSID.c_str());
				wifiMgr->setCredentials(deviceState.wifiSSID, deviceState.wifiPassword);
				LOG_DEBUG_COMPONENT("Startup", "WiFiManager: Calling checkSavedCredentials() to trigger auto-connect");
				wifiMgr->checkSavedCredentials();
			} else {
				LOG_DEBUGF_COMPONENT("Startup", "No WiFi credentials found - SSID length: %d", deviceState.wifiSSID.length());
			}
		}
	}

#else
	LOG_INFO("Preferences not supported on this platform - using defaults");
#endif

	// Initialize FreeRTOS LED update task
	// Note: Task can run even without SUPPORTS_LEDS - it will just sleep if no LED manager
	if (taskMgr.createLEDTask(16)) {  // 60 FPS
#if SUPPORTS_LEDS
		int numConfiguredLEDs = NUM_LEDS;
		if (settingsLoaded && settings._doc.containsKey("numLEDs"))
		{
			numConfiguredLEDs = settings._doc["numLEDs"].as<int>();
			LOG_DEBUGF_COMPONENT("Startup", "Setting numConfiguredLEDs to %d", numConfiguredLEDs);
			if (auto* ledTask = taskMgr.getLEDTask()) {
				ledTask->setNumConfiguredLEDs(numConfiguredLEDs);
			}
		}
		else
		{
			LOG_DEBUGF_COMPONENT("Startup", "No numLEDs found in settings, using default of %d", numConfiguredLEDs);
		}
#endif
	}

	// Initialize FreeRTOS system monitor task
	if (taskMgr.createSystemMonitorTask(1000)) {  // Every 1 second
#if SUPPORTS_POWER_SENSORS
		// Initialize power sensors in SystemMonitorTask
		if (auto* sysMon = taskMgr.getSystemMonitorTask()) {
			sysMon->initializePowerSensors(A2, A3, ACS712_30A, 5.0f, 3.3f);
#if ENABLE_POWER_SENSOR_CALIBRATION_DELAY
			// Force recalibration to ensure clean baseline
			sysMon->forceRecalibratePowerSensors();
#endif
		}
#endif
	}

	// Initialize FreeRTOS display task (OLED for non-CrowPanel platforms)
#if SUPPORTS_DISPLAY
	if (taskMgr.createOLEDDisplayTask(settingsLoaded ? &settings : nullptr, 200)) {
		LOG_INFO_COMPONENT("Startup", "FreeRTOS OLED display task created and started");
	} else {
		LOG_ERROR_COMPONENT("Startup", "Failed to create FreeRTOS OLED display task");
	}
#endif

	// Initialize LVGL display (for CrowPanel) - in main.cpp like CrowPanel project
#if PLATFORM_CROW_PANEL
	// Initialize display hardware
	LOG_INFO_COMPONENT("Startup", "Initializing LVGL display...");
	lcd.begin();
	// Test display hardware - fill with red to verify it works
	lcd.fillScreen(0xF800);  // Red (RGB565: 0xF800)
	delay(500);
	Serial.println("[LVGL] Display filled with red - do you see red?");
	delay(1000);
	lcd.fillScreen(0x0000);  // Black
	delay(200);
	screenWidth = lcd.width();
	screenHeight = lcd.height();
	LOG_INFOF_COMPONENT("Startup", "Display size: %dx%d", screenWidth, screenHeight);
	
	// Initialize LVGL
	lv_init();
	
	// Initialize display driver
	initLVGLDisplay();
	
	// Set up backlight
	#define TFT_BL 2
	pinMode(TFT_BL, OUTPUT);
	digitalWrite(TFT_BL, LOW);
	delay(500);
	digitalWrite(TFT_BL, HIGH);
	ledcSetup(1, 300, 8);
	ledcAttachPin(TFT_BL, 1);
	ledcWrite(1, 255);
	
	// Set up LVGL tick timer
	lvgl_tick_timer = timerBegin(0, 80, true);
	timerAttachInterrupt(lvgl_tick_timer, &lv_tick_cb, true);
	timerAlarmWrite(lvgl_tick_timer, 1000, true);
	timerAlarmEnable(lvgl_tick_timer);
	
	// Create UI
	createLVGLUI();
	
	// Force invalidate entire screen to trigger render
	Serial.println("[LVGL] Invalidating screen to force render...");
	if (screen) {
		lv_obj_invalidate(screen);
		lv_refr_now(nullptr);
	}
	
	// Initial render - call multiple times to ensure flush
	Serial.println("[LVGL] Calling lv_timer_handler() for initial render...");
	for (int i = 0; i < 20; i++) {
		lv_timer_handler();
		delay(5);
		if (i % 5 == 0) {
			Serial.printf("[LVGL] Render attempt %d/20\n", i+1);
		}
	}
	Serial.println("[LVGL] Initial render complete");
	
	LOG_INFO_COMPONENT("Startup", "LVGL display initialized");
#endif

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
	// Clean up all tasks
	TaskManager::getInstance().cleanupAll();

	// Cleanup SDCardAPI
#if SUPPORTS_SD_CARD
	SDCardAPI::cleanup();
	// LOG_INFO("SDCardAPI cleaned up");
#endif
}

#if SUPPORTS_LEDS
void DrawError(const CRGB &color)
{
	for (int i = 0; i < LEDS_MATRIX_X; i += 2)
	{
		leds[i] = color;
	}
}
#endif

void loop()
{
	static unsigned long lastLoopTime = millis();
	const auto now = millis();
	const auto dt = now - lastLoopTime;
	lastLoopTime = now;
	float dtSeconds = dt * 0.001f;

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
	// LoopOthers(0.16f);
	if (now - lastLogCheck > 5000)
	{
		lastLogCheck = now;
	}

#if PLATFORM_CROW_PANEL
	// Update LVGL display
	lv_timer_handler();
	
	// Update system stats on display periodically
	static unsigned long lastStatsUpdate = 0;
	if (now - lastStatsUpdate > 1000) {  // Update every second
		lastStatsUpdate = now;
		if (auto* sysMon = TaskManager::getInstance().getSystemMonitorTask()) {
			SystemStats stats = sysMon->getStats();
			uint32_t uptime = stats.uptimeSeconds;
			uint32_t days = uptime / 86400;
			uint32_t hours = (uptime % 86400) / 3600;
			uint32_t minutes = (uptime % 3600) / 60;
			uint32_t seconds = uptime % 60;
			char uptimeText[64];
			snprintf(uptimeText, sizeof(uptimeText), "Uptime: %u d %u h %u m %u s",
			         (unsigned int)days, (unsigned int)hours, 
			         (unsigned int)minutes, (unsigned int)seconds);
			if (uptimeLabel) lv_label_set_text(uptimeLabel, uptimeText);
			
			char heapText[64];
			snprintf(heapText, sizeof(heapText), "Heap: %d%% (%d KB free)",
			         stats.heapUsagePercent, stats.freeHeap / 1024);
			if (heapLabel) lv_label_set_text(heapLabel, heapText);
		}
	}
#endif

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
			if (auto* sysMon = TaskManager::getInstance().getSystemMonitorTask())
			{
				if (sysMon->forceRecalibratePowerSensors())
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
				LOG_ERROR("SystemMonitorTask not available for recalibration");
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
