#pragma once

#include <FastLED.h>
#include <stdint.h>
#include "../Globals.h"

class Pulser {
    enum _state : uint16_t {
        // First bit is for direction
        direction_forward = 0x01,
        direction_reverse = 0x00,
    };

    bool m_isActive = false;
    bool m_isForward = true;
    uint16_t m_currentIndex = 0;
    uint16_t m_minIndex = 0;
    uint16_t m_maxIndex = NUM_LEDS;
    CRGB m_currentColor;
    void_ftn_ptr m_onFinished = nullptr;
public:
    CRGB *leds;

    uint16_t GetCurrentIndex() const
    {
        return m_currentIndex;
    }

    // Update own internal state
    void Update(const CRGB &color)
    {
        if (!m_isActive) return;
        m_currentIndex += m_isForward ? 1 : -1;
        if (m_isForward && m_currentIndex >= m_maxIndex)
        {
            m_isForward = false;
        }
        else if (!m_isForward && m_currentIndex <= m_minIndex)
        {
            m_isForward = true;
            InvokeFrameEnd();
        }
        m_currentColor = color;
    }

    void InvokeFrameEnd()
    {
        if (m_onFinished)
        {
            m_onFinished();
        }
    }

    void Pause()
    {
        m_isActive = false;
    }

    // Write state to LEDs
    void Show()
    {
        if (!m_isActive) return;
        for (uint16_t i = m_minIndex; i < m_currentIndex && i < m_maxIndex; ++i)
        {
            leds[i] = m_currentColor;
        }
        for (uint16_t i = m_currentIndex; i < m_maxIndex; ++i)
        {
            leds[i] = CRGB::Black;
        }
    }

    void Start()
    {
        m_isActive = true;
        m_isForward = true;
        m_currentIndex = m_minIndex;
    }

    void Init(uint16_t minIdx, uint16_t maxIdx)
    {
        m_maxIndex = maxIdx;
        m_minIndex = minIdx;
    }

    void OnFinished(void_ftn_ptr ftn)
    {
        m_onFinished = ftn;
    }
};