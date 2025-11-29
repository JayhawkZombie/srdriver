#include "GlobalState.h"

// Brightness pulsing variables
bool isPulsing = false;
int pulseTargetBrightness = 0;
int previousBrightness = 0;
unsigned long pulseStartTime = 0;
unsigned long pulseDuration = 0;
bool isFadeMode = false;  // true for fade, false for pulse
bool isBooting = true;  // true during device startup, false after initialization

PreferencesManager prefsManager;
DeviceState deviceState;
