#pragma once
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include "Light.h"

// Safely format an int to a char buffer (up to 64 bytes, zero-initialized)
inline void safeIntToString(int value, char (&buf)[64]) {
    for (auto &c : buf) c = 0;
    snprintf(buf, sizeof(buf), "%d", value);
}

// Safely format a Light (r,g,b) to a char buffer (up to 64 bytes, zero-initialized)
inline void safeLightToString(const Light& color, char (&buf)[64]) {
    for (auto &c : buf) c = 0;
    snprintf(buf, sizeof(buf), "%d,%d,%d", color.r, color.g, color.b);
}

// General-purpose safe snprintf wrapper for custom formats
inline void safeFormatToString(char (&buf)[64], const char* fmt, ...) {
    for (auto &c : buf) c = 0;
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
}
