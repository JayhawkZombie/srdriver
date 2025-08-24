#pragma once
#include "PlatformConfig.h"
#include <ArduinoBLE.h>
#include "DeviceState.h"
#include <functional>
#include <vector>
#include "tasks/JsonChunkStreamer.h"
#include "hal/ble/BLECharacteristicRegistry.h"

// Forward declarations
class BLEStreamTask;

// Callback type for when a setting is changed via BLE
using OnSettingChangedCallback = void (*)(DeviceState&);

class BLEManager {
private:
    static BLEManager* instance;
    BLEManager(DeviceState& state, std::function<void(int)> goToPatternCb);
    
    DeviceState& deviceState;
    OnSettingChangedCallback onSettingChanged = nullptr;
    std::function<void(int)> goToPatternCallback;

    // BLE Service
    BLEService controlService;

    // BLE Characteristics
    // BLEStringCharacteristic brightnessCharacteristic; // Now managed by BrightnessController
    // BLEStringCharacteristic speedCharacteristic; // Now managed by SpeedController
    BLEStringCharacteristic patternIndexCharacteristic;
    BLEStringCharacteristic highColorCharacteristic;
    BLEStringCharacteristic lowColorCharacteristic;
    BLEStringCharacteristic leftSeriesCoefficientsCharacteristic;
    BLEStringCharacteristic rightSeriesCoefficientsCharacteristic;
    BLEStringCharacteristic commandCharacteristic;
    BLEUnsignedLongCharacteristic heartbeatCharacteristic;

    // BLE Descriptors
    // BLEDescriptor brightnessDescriptor; // Now managed by BrightnessController
    // BLEDescriptor speedDescriptor; // Now managed by SpeedController
    BLEDescriptor patternIndexDescriptor;
    BLEDescriptor highColorDescriptor;
    BLEDescriptor lowColorDescriptor;
    BLEDescriptor leftSeriesCoefficientsDescriptor;
    BLEDescriptor rightSeriesCoefficientsDescriptor;
    BLEDescriptor commandDescriptor;
    BLEDescriptor heartbeatDescriptor;

    // BLE Format Descriptors
    // BLEDescriptor brightnessFormatDescriptor; // Now managed by BrightnessController
    // BLEDescriptor speedFormatDescriptor; // Now managed by SpeedController
    BLEDescriptor patternIndexFormatDescriptor;
    BLEDescriptor highColorFormatDescriptor;
    BLEDescriptor lowColorFormatDescriptor;
    BLEDescriptor leftSeriesCoefficientsFormatDescriptor;
    BLEDescriptor rightSeriesCoefficientsFormatDescriptor;
    BLEDescriptor commandFormatDescriptor;
    BLEDescriptor heartbeatFormatDescriptor;

    // Handler registration
    struct CharacteristicHandler {
        BLECharacteristic* characteristic;
        std::function<void(const unsigned char* value)> onWrite;
    };
    std::vector<CharacteristicHandler> handlers;

    static BLE2904_Data stringFormat;
    static BLE2904_Data ulongFormat;

    void handleEvents();
    void updateCharacteristic(BLECharacteristic& characteristic, int value);
    void updateCharacteristic(BLECharacteristic& characteristic, const Light& color);

    JsonChunkStreamer jsonStreamer;
    
    // New registry
    BLECharacteristicRegistry registry;

public:
    // Singleton methods
    static BLEManager* getInstance() { return instance; }
    static void initialize(DeviceState& state, std::function<void(int)> goToPatternCb);
    static void destroy();
    
    // Call in setup()
    void begin();
    // Call this BEFORE begin() to register additional characteristics
    void registerCharacteristics();
    // Call in loop()
    // update() now also handles heartbeat updates internally
    void update();

    // Register a callback for when a setting is changed via BLE
    void setOnSettingChanged(OnSettingChangedCallback cb);
    
    // Trigger the OnSettingChanged callback manually
    void triggerOnSettingChanged();

    // Update all BLE characteristics to match device state
    void updateAllCharacteristics();

    void updateBrightness();

    // Stream data through BLE for large responses
    void streamData(const String& data);

    void startStreaming(const String& json, const String& type = "FILE_LIST");

    void sendFileDataChunk(const String& envelope);

    // Accessors for main.cpp
    // BLEStringCharacteristic& getBrightnessCharacteristic() { return brightnessCharacteristic; } // Now managed by BrightnessController
    // BLEStringCharacteristic& getSpeedCharacteristic() { return speedCharacteristic; } // Now managed by SpeedController
    BLEStringCharacteristic& getPatternIndexCharacteristic() { return patternIndexCharacteristic; }
    BLEStringCharacteristic& getHighColorCharacteristic() { return highColorCharacteristic; }
    BLEStringCharacteristic& getLowColorCharacteristic() { return lowColorCharacteristic; }
    BLEStringCharacteristic& getLeftSeriesCoefficientsCharacteristic() { return leftSeriesCoefficientsCharacteristic; }
    BLEStringCharacteristic& getRightSeriesCoefficientsCharacteristic() { return rightSeriesCoefficientsCharacteristic; }
    BLEStringCharacteristic& getCommandCharacteristic() { return commandCharacteristic; }
    BLEUnsignedLongCharacteristic& getHeartbeatCharacteristic() { return heartbeatCharacteristic; }
    
    // New registry access
    BLECharacteristicRegistry* getRegistry() { return &registry; }

#if SUPPORTS_SD_CARD
    BLEStringCharacteristic sdCardCommandCharacteristic;
    BLEStringCharacteristic sdCardStreamCharacteristic;
    BLEDescriptor sdCardCommandDescriptor;
    BLEDescriptor sdCardStreamDescriptor;
    BLEDescriptor sdCardCommandFormatDescriptor;
    BLEDescriptor sdCardStreamFormatDescriptor;
    BLEStringCharacteristic& getSDCardCommandCharacteristic() { return sdCardCommandCharacteristic; }
    BLEStringCharacteristic& getSDCardStreamCharacteristic() { return sdCardStreamCharacteristic; }
#endif
};
