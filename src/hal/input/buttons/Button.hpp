#pragma once
#include "Globals.h"

class Button
{
    private:
        int m_pin;
        int m_holdTimeMs;
        bool m_isPressed;
        bool m_eventProcessed;
        unsigned long m_pressedTime;

    public:
        enum class Event {
            NONE,           // No button event
            PRESS,          // Button was pressed and released quickly
            HOLD            // Button was held down and released
        };

        Button(int pin, int holdTimeMs = PUSHBUTTON_HOLD_TIME_MS)
        {
            m_pin = pin;
            m_holdTimeMs = holdTimeMs;
            m_isPressed = false;
            m_eventProcessed = false;
            m_pressedTime = 0;
        }

        Event getEvent()
        {
            if (digitalRead(m_pin) == LOW)
            {
                // Button is pressed
                if (!m_isPressed)
                {
                    // Button was just pressed
                    m_isPressed = true;
                    m_pressedTime = millis();
                    m_eventProcessed = false;
                }
            }
            else
            {
                // Button is released
                if (m_isPressed && !m_eventProcessed)
                {
                    m_isPressed = false;
                    m_eventProcessed = true;
                    
                    // If held long enough, it's a hold, otherwise it's a press
                    if (millis() - m_pressedTime >= m_holdTimeMs)
                    {
                        return Event::HOLD;
                    }
                    else
                    {
                        return Event::PRESS;
                    }
                }
            }
            return Event::NONE;
        }

        bool isPressed()
        {
            return digitalRead(m_pin) == LOW;
        }

        // For backward compatibility
        bool didPress()
        {
            return getEvent() == Event::PRESS;
        }

        bool isHeldDown(unsigned int minimumHoldTimeMs = 0)
        {
            return getEvent() == Event::HOLD;
        }
};
