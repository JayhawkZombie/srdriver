#pragma once
#include <ArduinoBLE.h>
#include "Light.h"
#include <cstdio>
#include <cstdint>
#include <cstdarg>

// Generic template: not implemented, will cause a compile error for unsupported types
// Usage: writeToCharacteristic(characteristic, value);
template<typename T>
void writeToCharacteristic(BLECharacteristic& characteristic, const T& value);

// Specialization for int
template<>
inline void writeToCharacteristic<int>(BLECharacteristic& characteristic, const int& value) {
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "%d", value);
    characteristic.writeValue(buf);
}

// Specialization for Light
template<>
inline void writeToCharacteristic<Light>(BLECharacteristic& characteristic, const Light& value) {
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "%d,%d,%d", value.r, value.g, value.b);
    characteristic.writeValue(buf);
}

// You can add more specializations for other types as needed.
