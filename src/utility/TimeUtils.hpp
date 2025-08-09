#pragma once

#include <Arduino.h>

using time_ms_t = decltype(millis());

inline time_ms_t MsToSeconds(time_ms_t ms) {
    return ms / 1000;
}

inline time_ms_t SecondsToMs(int seconds) {
    return seconds * 1000;
}

inline time_ms_t MinutesToMs(int minutes) {
    return minutes * 60 * 1000;
}

inline time_ms_t HoursToMs(int hours) {
    return hours * 60 * 60 * 1000;
}

inline time_ms_t DaysToMs(int days) {
    return days * 24 * 60 * 60 * 1000;
}
