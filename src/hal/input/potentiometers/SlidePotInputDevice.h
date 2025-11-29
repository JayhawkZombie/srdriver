#pragma once

#include "../InputDevice.h"
#include "SlidePot.h"

/**
 * Input device implementation for slide potentiometers using the slidePot class
 * Provides better filtering with hysteresis, bit shifting, and bump counting
 */
class SlidePotInputDevice : public InputDevice {
private:
    slidePot* pot;
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
     * @param bitShift Bit shift for noise reduction (default: 3)
     * @param minDiff Minimum difference threshold (default: 1)
     * @param bumpLimit Number of consecutive readings required (default: 3)
     */
    SlidePotInputDevice(const String& deviceName, int potPin, int bitShift = 3, int minDiff = 1, int bumpLimit = 3) 
        : name(deviceName), changed(false), initialized(false) {
        pot = new slidePot(potPin, bitShift, minDiff, bumpLimit);
        initialized = true;
        lastValue = pot->update();  // Get initial value
        currentValue = lastValue;
        
        LOG_DEBUGF("SlidePot %s created with pin: %d, bitShift: %d, minDiff: %d, bumpLimit: %d", 
                   deviceName.c_str(), potPin, bitShift, minDiff, bumpLimit);
    }
    
    /**
     * Destructor
     */
    ~SlidePotInputDevice() {
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
        int newValue = pot->update();
        
        // Check if value changed (slidePot.update() returns the steadyVal)
        changed = (newValue != currentValue);
        currentValue = newValue;
        
        // Debug logging
        static int debugCounter = 0;
        if (debugCounter++ % 100 == 0) {  // Log every 100 polls
            LOG_DEBUGF("SlidePot %s - Raw: %d, Steady: %d, Changed: %s, BumpCnt: %d", 
                name.c_str(), analogRead(pot->pinID), newValue, changed ? "YES" : "NO", pot->bumpCnt);
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
     * @return "slide_potentiometer"
     */
    String getDeviceType() const override { 
        return "slide_potentiometer"; 
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
    int getRawValue() const {
        return pot ? pot->steadyVal : 0;
    }
    
    /**
     * Get mapped value (0-255)
     * @return Mapped potentiometer value
     */
    int getMappedValue() const {
        return getMappedValue(0, 255);
    }
    
    /**
     * Get current value with filtering
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
        if (!pot) return low;
        int value = pot->steadyVal;
        // Map from 0-4095 to low-high
        value = (value * (high - low)) / 4095 + low;
        return value;
    }
    
    /**
     * Get curve-mapped value (0.0-1.0)
     * @return Curve-mapped value
     */
    float getCurveMappedValue() const {
        if (!pot) return 0.0f;
        float normalizedValue = pot->steadyVal / 4095.0f;
        // Simple exponential curve
        return normalizedValue * normalizedValue;
    }
    
    /**
     * Set minimum difference threshold
     * @param minDiff New minimum difference threshold
     */
    void setMinDiff(int minDiff) {
        if (pot) {
            pot->minDiff = minDiff;
            LOG_DEBUGF("SlidePot %s: setMinDiff(%d) - pot->minDiff now: %d", name.c_str(), minDiff, pot->minDiff);
        }
    }
    
    /**
     * Get minimum difference threshold
     * @return Current minimum difference threshold
     */
    int getMinDiff() const {
        return pot ? pot->minDiff : 1;
    }
    
    /**
     * Set bit shift for noise reduction
     * @param bitShift New bit shift value
     */
    void setBitShift(int bitShift) {
        if (pot) {
            pot->bitShift = bitShift;
            LOG_DEBUGF("SlidePot %s: setBitShift(%d) - pot->bitShift now: %d", name.c_str(), bitShift, pot->bitShift);
        }
    }
    
    /**
     * Get bit shift value
     * @return Current bit shift value
     */
    int getBitShift() const {
        return pot ? pot->bitShift : 3;
    }
    
    /**
     * Set bump limit
     * @param bumpLimit New bump limit value
     */
    void setBumpLimit(int bumpLimit) {
        if (pot) {
            pot->bumpLimit = bumpLimit;
            LOG_DEBUGF("SlidePot %s: setBumpLimit(%d) - pot->bumpLimit now: %d", name.c_str(), bumpLimit, pot->bumpLimit);
        }
    }
    
    /**
     * Get bump limit
     * @return Current bump limit value
     */
    int getBumpLimit() const {
        return pot ? pot->bumpLimit : 3;
    }
    
    /**
     * Get current bump count
     * @return Current bump count
     */
    int getBumpCount() const {
        return pot ? pot->bumpCnt : 0;
    }
};
