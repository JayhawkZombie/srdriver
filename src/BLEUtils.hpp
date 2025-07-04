#pragma once
#include <ArduinoBLE.h>
#include "Light.h"
#include <cstdio>
#include <cstdint>
#include <cstdarg>

inline int parseIntFromBLEValue(const unsigned char* value) {
    if (!value) return 0;
    return atoi(reinterpret_cast<const char*>(value));
}
