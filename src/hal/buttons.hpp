#pragma once

#include "Globals.h"

enum class ButtonEvent {
    NONE,           // No button event
    PRESS,          // Button was pressed and released quickly
    HOLD            // Button was held down and released
};

// Internal state tracking
namespace {
    bool buttonPressed = false;
    unsigned long buttonPressedTime = 0;
    bool eventProcessed = false;  // Track if we've processed the current button event
}

ButtonEvent GetButtonEvent()
{
    if (digitalRead(PUSHBUTTON_PIN) == LOW)
    {
        // Button is pressed
        if (!buttonPressed)
        {
            // Button was just pressed
            buttonPressed = true;
            buttonPressedTime = millis();
            eventProcessed = false;
        }
    }
    else
    {
        // Button is released
        if (buttonPressed && !eventProcessed)
        {
            buttonPressed = false;
            eventProcessed = true;
            
            // If held long enough, it's a hold, otherwise it's a press
            if (millis() - buttonPressedTime >= PUSHBUTTON_HOLD_TIME_MS)
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
bool DidPushButton()
{
    return GetButtonEvent() == ButtonEvent::PRESS;
}

bool IsButtonHeldDown(unsigned int minimumHoldTimeMs)
{
    return GetButtonEvent() == ButtonEvent::HOLD;
}
