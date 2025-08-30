#pragma once

#include "GlobalState.h"
#include "hal/ble/BLECharacteristicRegistry.h"
#include <functional>

class BrightnessController {
private:
    static BrightnessController *instance;
    BrightnessController();

    // Internal state
    int currentBrightness;
    int targetBrightness;
    bool isPulsing;
    unsigned long pulseStartTime;
    unsigned long pulseDuration;
    bool isFadeMode;

    // Callbacks for external systems
    std::function<void(int)> onBrightnessChanged;
    std::function<void()> onPulseComplete;

    // BLE characteristic info
    BLECharacteristicInfo brightnessCharacteristicInfo;

public:
    static BrightnessController *getInstance();
    static void initialize();
    static void destroy();

    // Core brightness management
    void setBrightness(int brightness);
    int getBrightness() const { return currentBrightness; }

    // Pulse/fade functionality
    void startPulse(int targetBrightness, unsigned long duration);
    void startFade(int targetBrightness, unsigned long duration);
    void stopPulse();
    bool isPulsingActive() const { return isPulsing; }

    // Update loop (called from main loop)
    void update();

    // Callback registration
    void setOnBrightnessChanged(std::function<void(int)> callback) { onBrightnessChanged = callback; }
    void setOnPulseComplete(std::function<void()> callback) { onPulseComplete = callback; }

    // BLE registration
    void registerBLECharacteristic();
    void unregisterBLECharacteristic();

    // Integration with device state
    void syncWithDeviceState(DeviceState &deviceState);
    void updateDeviceState(DeviceState &deviceState);
};