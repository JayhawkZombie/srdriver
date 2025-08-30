#pragma once

#include "../InputDevice.h"
#include "buttons.hpp"

/**
 * Input device implementation for buttons
 * Handles digital input with press/hold/release detection
 */
class ButtonInputDevice : public InputDevice {
private:
    int pin;
    String name;
    ButtonEvent lastEvent;
    ButtonEvent currentEvent;
    bool changed;
    bool initialized;
    
public:
    /**
     * Constructor
     * @param deviceName Name of the device
     * @param buttonPin GPIO pin number
     */
    ButtonInputDevice(const String& deviceName, int buttonPin) 
        : pin(buttonPin), name(deviceName), changed(false), initialized(false) {
        pinMode(pin, INPUT_PULLUP);
        initialized = true;
        lastEvent = ButtonEvent::NONE;
        currentEvent = ButtonEvent::NONE;
    }
    
    /**
     * Poll the button for current state
     */
    void poll() override {
        if (!initialized) return;
        
        lastEvent = currentEvent;
        currentEvent = GetButtonEvent(pin);  // Use updated button logic with pin parameter
        
        // Check if event changed
        changed = (currentEvent != lastEvent && currentEvent != ButtonEvent::NONE);
    }
    
    /**
     * Check if button state has changed
     * @return true if state changed, false otherwise
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
     * @return "button"
     */
    String getDeviceType() const override { 
        return "button"; 
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
     * @return Pointer to current ButtonEvent
     */
    void* getEventData() override { 
        return &currentEvent; 
    }
    
    /**
     * Get raw value (0 = not pressed, 1 = pressed)
     * @return Raw button state
     */
    int getRawValue() const override {
        return (currentEvent != ButtonEvent::NONE) ? 1 : 0;
    }
    
    /**
     * Get mapped value (0 = not pressed, 1 = pressed)
     * @return Mapped button state
     */
    int getMappedValue() const override {
        return getRawValue();
    }
    
    /**
     * Get current button event
     * @return Current ButtonEvent
     */
    ButtonEvent getCurrentEvent() const {
        return currentEvent;
    }
    
    /**
     * Get last button event
     * @return Last ButtonEvent
     */
    ButtonEvent getLastEvent() const {
        return lastEvent;
    }
    
    /**
     * Check if button is currently pressed
     * @return true if pressed, false otherwise
     */
    bool isPressed() const {
        return (currentEvent != ButtonEvent::NONE);
    }
    
    /**
     * Check if button was just pressed
     * @return true if just pressed, false otherwise
     */
    bool wasJustPressed() const {
        return (currentEvent == ButtonEvent::PRESS);
    }
    
    /**
     * Check if button was just held
     * @return true if just held, false otherwise
     */
    bool wasJustHeld() const {
        return (currentEvent == ButtonEvent::HOLD);
    }
}; 