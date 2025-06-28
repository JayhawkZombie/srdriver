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
    uint16_t m_holdCount = 0;
    uint16_t m_currentHold = 0;

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
        if (m_currentHold != 0)
        {
            m_currentHold--;
            return;
        }

        m_currentIndex += m_isForward ? 1 : -1;
        if (m_isForward && m_currentIndex >= m_maxIndex)
        {
            m_isForward = false;
            InvokeFrameEnd();
        }
        else if (!m_isForward && m_currentIndex <= m_minIndex)
        {
            // This is where we flip and go back "up" the strip.
            // Want to hold when fully filled. If m_holdCount = 0
            // then next iteration will go as it should normally... I hope
            m_isForward = true;
            m_currentHold = m_holdCount;
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
        // m_currentHold = 0;
    }

    void Resume()
    {
        m_isActive = true;
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
        m_currentHold = 0;
    }

    void SetHold(uint16_t hold) {
        m_holdCount = hold;
    }

    void Init(uint16_t minIdx, uint16_t maxIdx)
    {
        m_maxIndex = maxIdx;
        m_minIndex = minIdx;
        m_currentIndex = m_maxIndex;
        m_currentHold = 0;
    }
};