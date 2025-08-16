#pragma once

#include <Arduino.h>
#include "hal/ble/BLECharacteristicRegistry.h"
#include "DeviceState.h"

class SpeedController {
private:
    static SpeedController* instance;
    float currentSpeed;
    float targetSpeed;
    bool isTransitioning;
    unsigned long transitionStartTime;
    unsigned long transitionDuration;
    
    // BLE characteristic info
    BLECharacteristicInfo speedCharacteristicInfo;
    
public:
    // Singleton methods
    static SpeedController* getInstance();
    static void initialize();
    static void destroy();
    
    // Speed control
    void setSpeed(float speed);
    void setSpeedWithTransition(float targetSpeed, unsigned long duration);
    float getSpeed() const { return currentSpeed; }
    void stopTransition();
    
    // BLE integration
    void registerBLECharacteristic();
    void unregisterBLECharacteristic();
    
    // Update loop
    void update();
    
    // State management
    void syncWithDeviceState(DeviceState& deviceState);
    void updateDeviceState(DeviceState& deviceState);
    
private:
    SpeedController(); // Private constructor for singleton
    void updateSpeed(float newSpeed);
}; 