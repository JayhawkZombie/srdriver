#pragma once

#include <Arduino.h>

/**
 * Types of input events that can be generated
 */
enum class InputEventType {
    BUTTON_PRESS,
    BUTTON_HOLD,
    BUTTON_RELEASE,
    POTENTIOMETER_CHANGE,
    MICROPHONE_AUDIO_DETECTED,
    MICROPHONE_CLIPPING,
    GENERIC_VALUE_CHANGE
};

/**
 * Structure containing input event data
 * Used for communication between tasks and callback handling
 */
struct InputEvent {
    String deviceName;
    InputEventType eventType;
    int value;           // Raw value
    int mappedValue;     // Mapped value (0-255, etc.)
    uint32_t timestamp;
    String eventData;    // JSON string for complex data
    
    InputEvent() : value(0), mappedValue(0), timestamp(0) {}
    
    InputEvent(const String& name, InputEventType type, int val, int mappedVal, uint32_t time)
        : deviceName(name), eventType(type), value(val), mappedValue(mappedVal), timestamp(time) {}
}; 