#pragma once

#include "Globals.h"

enum class ButtonEvent {
    NONE,           // No button event
    PRESS,          // Button was pressed and released quickly
    HOLD            // Button was held down and released
};

// Internal state tracking - now supports multiple buttons
namespace {
    struct ButtonState {
        bool buttonPressed = false;
        unsigned long buttonPressedTime = 0;
        bool eventProcessed = false;
    };
    
    // Map to track state for each pin
    std::map<int, ButtonState> buttonStates;
}

inline ButtonEvent GetButtonEvent(int pin = PUSHBUTTON_PIN)
{
    // Get or create state for this pin
    ButtonState& state = buttonStates[pin];
    
    if (digitalRead(pin) == LOW)
    {
        // Button is pressed
        if (!state.buttonPressed)
        {
            // Button was just pressed
            state.buttonPressed = true;
            state.buttonPressedTime = millis();
            state.eventProcessed = false;
        }
    }
    else
    {
        // Button is released
        if (state.buttonPressed && !state.eventProcessed)
        {
            state.buttonPressed = false;
            state.eventProcessed = true;
            
            // If held long enough, it's a hold, otherwise it's a press
            if (millis() - state.buttonPressedTime >= PUSHBUTTON_HOLD_TIME_MS)
            {
                return ButtonEvent::HOLD;
            }
            else
            {
                return ButtonEvent::PRESS;
            }
        }
    }
    return ButtonEvent::NONE;
}

// For backward compatibility
inline ButtonEvent GetButtonEvent()
{
    return GetButtonEvent(PUSHBUTTON_PIN);
}

inline bool DidPushButton()
{
    return GetButtonEvent(PUSHBUTTON_PIN) == ButtonEvent::PRESS;
}

inline bool IsButtonHeldDown(unsigned int minimumHoldTimeMs)
{
    return GetButtonEvent(PUSHBUTTON_PIN) == ButtonEvent::HOLD;
}
