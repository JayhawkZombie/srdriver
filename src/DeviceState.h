#pragma once
#include "../lights/Light.h"

struct DeviceState {
    int brightness = 128;
    int patternIndex = 0;
    int currentWavePlayerIndex = 0;
    Light highColor = Light(255, 255, 255);
    Light lowColor = Light(0, 0, 0);
    float speedMultiplier = 1.0f;
    // Series coefficients for wave patterns
    float leftSeriesCoefficients[3] = {0.0f, 0.0f, 0.0f};
    float rightSeriesCoefficients[3] = {0.0f, 0.0f, 0.0f};
    int numTermsLt = 0;
    int numTermsRt = 0;
    // Add more fields as needed (e.g., speedMultiplier, etc)
}; 