#pragma once

#include "../InputDevice.h"
#include "Potentiometer.hpp"

/**
 * Input device implementation for potentiometers
 * Handles analog input with hysteresis and value mapping
 */
class PotentiometerInputDevice : public InputDevice {
private:
    Potentiometer* pot;
    String name;
    int lastValue;
    int currentValue;
    bool changed;
    bool initialized;
    
public:
    /**
     * Constructor
     * @param deviceName Name of the device
     * @param potPin Analog pin number
     */
    PotentiometerInputDevice(const String& deviceName, int potPin) 
        : name(deviceName), changed(false), initialized(false) {
        pot = new Potentiometer(potPin);
        initialized = true;
        lastValue = pot->getValue();
        currentValue = lastValue;
    }
    
    /**
     * Destructor
     */
    ~PotentiometerInputDevice() {
        if (pot) {
            delete pot;
            pot = nullptr;
        }
    }
    
    /**
     * Poll the potentiometer for current state
     */
    void poll() override {
        if (!initialized || !pot) return;
        
        lastValue = currentValue;
        currentValue = pot->getValue();
        
        // Check if value changed (potentiometer handles hysteresis internally)
        changed = pot->hasChanged();
        if (changed) {
            pot->resetChanged();
        }
    }
    
    /**
     * Check if potentiometer value has changed
     * @return true if value changed, false otherwise
     */
    bool hasChanged() const override { 
        return changed; 
    }
    
    /**
     * Reset the changed flag
     */
    void resetChanged() override { 
        changed = false; 
    }
    
    /**
     * Get device type
     * @return "potentiometer"
     */
    String getDeviceType() const override { 
        return "potentiometer"; 
    }
    
    /**
     * Get device name
     * @return Device name
     */
    String getName() const override { 
        return name; 
    }
    
    /**
     * Get raw event data
     * @return Pointer to current value
     */
    void* getEventData() override { 
        return &currentValue; 
    }
    
    /**
     * Get raw value (0-4095 for ESP32)
     * @return Raw potentiometer value
     */
    int getRawValue() const override {
        return pot ? pot->getRawValue() : 0;
    }
    
    /**
     * Get mapped value (0-255)
     * @return Mapped potentiometer value
     */
    int getMappedValue() const override {
        return pot ? pot->getMappedValue(0, 255) : 0;
    }
    
    /**
     * Get current value with hysteresis
     * @return Current potentiometer value
     */
    int getValue() const {
        return currentValue;
    }
    
    /**
     * Get last value
     * @return Last potentiometer value
     */
    int getLastValue() const {
        return lastValue;
    }
    
    /**
     * Get mapped value with custom range
     * @param low Lower bound
     * @param high Upper bound
     * @return Mapped value in specified range
     */
    int getMappedValue(int low, int high) const {
        return pot ? pot->getMappedValue(low, high) : low;
    }
    
    /**
     * Get curve-mapped value (0.0-1.0)
     * @return Curve-mapped value
     */
    float getCurveMappedValue() const {
        return pot ? pot->getCurveMappedValue() : 0.0f;
    }
    
    /**
     * Set hysteresis threshold
     * @param threshold Hysteresis threshold value
     */
    void setHysteresisThreshold(int threshold) {
        if (pot) {
            pot->setHysteresisThreshold(threshold);
        }
    }
    
    /**
     * Get hysteresis threshold
     * @return Current hysteresis threshold
     */
    int getHysteresisThreshold() const {
        return pot ? pot->getHysteresisThreshold() : 50;
    }
}; 