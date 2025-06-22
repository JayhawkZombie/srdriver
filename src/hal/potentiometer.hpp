#pragma once
#include "Globals.h"

class Potentiometer
{
    private:
        int m_pin;
        int m_maxValue;
        int m_lastValue;
        bool m_hasChanged;

    public:
        Potentiometer(int potentiometerPin, int maxValue = 4095) 
        {
            m_pin = potentiometerPin;
            m_maxValue = maxValue;
            m_lastValue = 0;
            m_hasChanged = false;
        }

        int getRawValue()
        {
            return analogRead(m_pin);
        }

        int getValue()
        {
            int value = getRawValue();
            if (value != m_lastValue)
            {
                m_lastValue = value;
                m_hasChanged = true;
                return value;
            }
            m_hasChanged = false;
            return m_lastValue;
        }

        int getMappedValue(int low, int high)
        {
            int value = getValue();
            // Map from 0-maxValue to low-high
            value = (value * (high - low)) / m_maxValue + low;
            return value;
        }
        
        bool hasChanged()
        {
            return m_hasChanged;
        }

        int getLastValue()
        {
            return m_lastValue;
        }

        // For backward compatibility - matches the existing function signature
        int getMappedValue(int low, int high, int potentiometerMaxValue)
        {
            int value = getValue();
            // Map from 0-potentiometerMaxValue to low-high
            value = (value * (high - low)) / potentiometerMaxValue + low;
            return value;
        }
};
