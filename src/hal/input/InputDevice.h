#pragma once

#include <Arduino.h>

/**
 * Base interface for all input devices
 * Provides a common interface for polling and state management
 */
class InputDevice {
public:
    virtual ~InputDevice() = default;
    
    /**
     * Poll the device for current state
     * Should be called regularly by the task
     */
    virtual void poll() = 0;
    
    /**
     * Check if device state has changed since last poll
     * @return true if state changed, false otherwise
     */
    virtual bool hasChanged() const = 0;
    
    /**
     * Reset the changed flag after processing
     */
    virtual void resetChanged() = 0;
    
    /**
     * Get the device type as a string
     * @return Device type (e.g., "button", "potentiometer", "microphone")
     */
    virtual String getDeviceType() const = 0;
    
    /**
     * Get the device name
     * @return Device name as configured
     */
    virtual String getName() const = 0;
    
    /**
     * Get raw event data (device-specific)
     * @return Pointer to device-specific event data
     */
    virtual void* getEventData() = 0;
    
    /**
     * Get the current raw value
     * @return Raw device value
     */
    virtual int getRawValue() const = 0;
    
    /**
     * Get the current mapped value (0-255, etc.)
     * @return Mapped device value
     */
    virtual int getMappedValue() const = 0;
}; 