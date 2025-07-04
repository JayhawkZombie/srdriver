#pragma once
#include <ArduinoBLE.h>
#include "DeviceState.h"

// Callback type for when a setting is changed via BLE
using OnSettingChangedCallback = void (*)(DeviceState&);

class BLEManager {
public:
    // Construct with a reference to the global device state
    BLEManager(DeviceState& state);

    // Call in setup()
    void begin();
    // Call in loop()
    void poll();

    // Register a callback for when a setting is changed via BLE
    void setOnSettingChanged(OnSettingChangedCallback cb);

    // Update all BLE characteristics to match device state
    void updateAllCharacteristics();

private:
    DeviceState& deviceState;
    OnSettingChangedCallback onSettingChanged = nullptr;

    void setupCharacteristics();
    void handleEvents();
    void updateCharacteristic(BLECharacteristic& characteristic, int value);
    void updateCharacteristic(BLECharacteristic& characteristic, const Light& color);
};
