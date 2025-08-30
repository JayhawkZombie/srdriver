#pragma once

#include "Globals.h"

// 12-bit ADC, 0-4095
int GetPotentiometerValue()
{
    static int lastValue = 0;
    int value = analogRead(POTENTIOMETER_PIN_BRIGHTNESS);
    if (value != lastValue)
    {
        lastValue = value;
        return value;
    }
    // Serial.println("Potentiometer value: " + String(value));
    return lastValue;
}

int GetMappedPotentiometerValue(int low, int high, int potentiometerMaxValue)
{
    int value = GetPotentiometerValue();
    // Map from 0-1023 to low-high, don't use builtins
    value = (value * (high - low)) / potentiometerMaxValue + low;
    return value;
}
