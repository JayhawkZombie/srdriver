#pragma once

#include <FastLED.h>
#include <stdint.h>
#include "../Globals.h"

class ReversePulser {
    enum _state : uint16_t {
        // First bit is for direction
        direction_forward = 0x01,
        direction_reverse = 0x00,
    };

    bool m_isForward = true;
    bool m_isActive = false;
    uint16_t m_currentIndex = 0;
    uint16_t m_minIndex = 0;
    uint16_t m_maxIndex = NUM_LEDS;
    CRGB m_currentColor;
    void_ftn_ptr m_onFinished;

    void InvokeFrameEnd()
    {
        if (m_onFinished)
        {
            m_onFinished();
        }
    }

public:
    CRGB *leds;

    // Update own internal state
    void Update(const CRGB &color)
    {
        if (!m_isActive) return;
        m_currentIndex += m_isForward ? 1 : -1;
        if (m_isForward && m_currentIndex >= m_maxIndex)
        {
            m_isForward = false;
            InvokeFrameEnd();
        }
        else if (!m_isForward && m_currentIndex <= m_minIndex)
        {
            m_isForward = true;
        }
        m_currentColor = color;
    }

    void OnFinished(void_ftn_ptr ftn)
    {
        m_onFinished = ftn;
    }

    void Pause()
    {
        m_isActive = false;
    }

    // Write state to LEDs
    void Show()
    {
        if (!m_isActive) return;
        for (uint16_t i = m_minIndex; i < m_currentIndex; ++i)
        {
            leds[i] = CRGB::Black;
        }
        for (uint16_t i = m_currentIndex; i < m_maxIndex; ++i)
        {
            leds[i] = m_currentColor;
        }
    }

    void Start()
    {
        m_isForward = false;
        m_currentIndex = m_maxIndex;
        m_isActive = true;
    }

    void Init(uint16_t minIdx, uint16_t maxIdx)
    {
        m_maxIndex = maxIdx;
        m_minIndex = minIdx;
        m_currentIndex = m_maxIndex;
    }
};