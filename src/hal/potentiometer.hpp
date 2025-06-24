#pragma once
#include "Globals.h"

class Potentiometer
{
    private:
        int m_pin;
        int m_maxValue;
        int m_lastValue;
        bool m_hasChanged;
        int m_hysteresisThreshold;

        // Argument intended to be a value between 0 and 1
        float getCurveMappedValue(float value)
        {
            const auto numerator = exp(value) - 1.f;
            const auto denominator = exp(1.f) - 1.f;
            return numerator / denominator;
        }

        float getVaryingCurveMappedValue(float constant, float value)
        {
            const auto numerator = exp(constant * value) - 1.f;
            const auto denominator = exp(constant) - 1.f;
            return numerator / denominator;
        }

    public:
        Potentiometer(int potentiometerPin, int maxValue = 4095) 
        {
            m_pin = potentiometerPin;
            m_maxValue = maxValue;
            m_lastValue = 0;
            m_hasChanged = false;
            m_hysteresisThreshold = 50; // Default hysteresis threshold
        }

        int getRawValue()
        {
            return analogRead(m_pin);
        }

        int getValue()
        {
            int value = getRawValue();
            if (abs(value - m_lastValue) > m_hysteresisThreshold)
            {
                m_lastValue = value;
                m_hasChanged = true;
                return value;
            }
            return m_lastValue;
        }

        int getMappedValue(int low, int high)
        {
            int value = getRawValue(); // Use raw value for mapping
            // Map from 0-maxValue to low-high
            value = (value * (high - low)) / m_maxValue + low;
            return value;
        }
        
        bool hasChanged()
        {
            return m_hasChanged;
        }

        void resetChanged()
        {
            m_hasChanged = false;
        }

        void setHysteresisThreshold(int threshold)
        {
            m_hysteresisThreshold = threshold;
        }

        int getLastValue()
        {
            return m_lastValue;
        }

        float getCurveMappedValue()
        {
            // Return a value between 0 and 1, mapped using the curve
            int currentValue = getRawValue(); // Use raw value for mapping
            // float mappedValue = getCurveMappedValue(currentValue / (float)m_maxValue);
            float mappedValue = getVaryingCurveMappedValue(0.5f, currentValue / (float)m_maxValue);
            return mappedValue;
        }

        // For backward compatibility - matches the existing function signature
        int getMappedValue(int low, int high, int potentiometerMaxValue)
        {
            int value = getRawValue(); // Use raw value for mapping
            // Map from 0-potentiometerMaxValue to low-high
            value = (value * (high - low)) / potentiometerMaxValue + low;
            return value;
        }
};
