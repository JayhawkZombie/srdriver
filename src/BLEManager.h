#pragma once
#include <ArduinoBLE.h>
#include "DeviceState.h"
#include <functional>
#include <vector>
#include "tasks/JsonChunkStreamer.h"

// Forward declarations
class Scheduler;
class BLEStreamTask;

// Callback type for when a setting is changed via BLE
using OnSettingChangedCallback = void (*)(DeviceState&);

struct BLE2904_Data {
    uint8_t m_format;
    int8_t m_exponent;
    uint16_t m_unit;
    uint8_t m_namespace;
    uint16_t m_description;
} __attribute__((packed));

class BLEManager {
public:
    // Construct with a reference to the global device state
    BLEManager(DeviceState& state, std::function<void(int)> goToPatternCb);
    // Call in setup()
    void begin();
    // Call in loop()
    // update() now also handles heartbeat updates internally
    void update();

    // Register a callback for when a setting is changed via BLE
    void setOnSettingChanged(OnSettingChangedCallback cb);

    // Update all BLE characteristics to match device state
    void updateAllCharacteristics();

    void updateBrightness();

    // Stream data through BLE for large responses
    void streamData(const String& data);

    void startStreaming(const String& json, const String& type = "FILE_LIST");

    // Accessors for main.cpp
    BLEStringCharacteristic& getBrightnessCharacteristic() { return brightnessCharacteristic; }
    BLEStringCharacteristic& getSpeedCharacteristic() { return speedCharacteristic; }
    BLEStringCharacteristic& getPatternIndexCharacteristic() { return patternIndexCharacteristic; }
    BLEStringCharacteristic& getHighColorCharacteristic() { return highColorCharacteristic; }
    BLEStringCharacteristic& getLowColorCharacteristic() { return lowColorCharacteristic; }
    BLEStringCharacteristic& getLeftSeriesCoefficientsCharacteristic() { return leftSeriesCoefficientsCharacteristic; }
    BLEStringCharacteristic& getRightSeriesCoefficientsCharacteristic() { return rightSeriesCoefficientsCharacteristic; }
    BLEStringCharacteristic& getCommandCharacteristic() { return commandCharacteristic; }
    BLEUnsignedLongCharacteristic& getHeartbeatCharacteristic() { return heartbeatCharacteristic; }
    BLEStringCharacteristic& getSDCardCommandCharacteristic() { return sdCardCommandCharacteristic; }
    BLEStringCharacteristic& getSDCardStreamCharacteristic() { return sdCardStreamCharacteristic; }

private:
    DeviceState& deviceState;
    OnSettingChangedCallback onSettingChanged = nullptr;
    std::function<void(int)> goToPatternCallback;

    // BLE Service
    BLEService controlService;

    // BLE Characteristics
    BLEStringCharacteristic brightnessCharacteristic;
    BLEStringCharacteristic speedCharacteristic;
    BLEStringCharacteristic patternIndexCharacteristic;
    BLEStringCharacteristic highColorCharacteristic;
    BLEStringCharacteristic lowColorCharacteristic;
    BLEStringCharacteristic leftSeriesCoefficientsCharacteristic;
    BLEStringCharacteristic rightSeriesCoefficientsCharacteristic;
    BLEStringCharacteristic commandCharacteristic;
    BLEUnsignedLongCharacteristic heartbeatCharacteristic;
    BLEStringCharacteristic sdCardCommandCharacteristic;
    BLEStringCharacteristic sdCardStreamCharacteristic;

    // BLE Descriptors
    BLEDescriptor brightnessDescriptor;
    BLEDescriptor speedDescriptor;
    BLEDescriptor patternIndexDescriptor;
    BLEDescriptor highColorDescriptor;
    BLEDescriptor lowColorDescriptor;
    BLEDescriptor leftSeriesCoefficientsDescriptor;
    BLEDescriptor rightSeriesCoefficientsDescriptor;
    BLEDescriptor commandDescriptor;
    BLEDescriptor heartbeatDescriptor;
    BLEDescriptor sdCardCommandDescriptor;
    BLEDescriptor sdCardStreamDescriptor;

    // BLE Format Descriptors
    BLEDescriptor brightnessFormatDescriptor;
    BLEDescriptor speedFormatDescriptor;
    BLEDescriptor patternIndexFormatDescriptor;
    BLEDescriptor highColorFormatDescriptor;
    BLEDescriptor lowColorFormatDescriptor;
    BLEDescriptor leftSeriesCoefficientsFormatDescriptor;
    BLEDescriptor rightSeriesCoefficientsFormatDescriptor;
    BLEDescriptor commandFormatDescriptor;
    BLEDescriptor heartbeatFormatDescriptor;
    BLEDescriptor sdCardCommandFormatDescriptor;
    BLEDescriptor sdCardStreamFormatDescriptor;

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
};
