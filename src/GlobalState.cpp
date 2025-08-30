#include "GlobalState.h"

String authorizedDevices[MAX_AUTHORIZED_DEVICES];
int numAuthorizedDevices = 0;
bool isAuthenticated = false;
bool pairingMode = false;
unsigned long pairingModeStartTime = 0;
const unsigned long PAIRING_TIMEOUT = 30000; // 30 seconds

// Brightness pulsing variables
bool isPulsing = false;
int pulseTargetBrightness = 0;
int previousBrightness = 0;
unsigned long pulseStartTime = 0;
unsigned long pulseDuration = 0;
bool isFadeMode = false;  // true for fade, false for pulse

PreferencesManager prefsManager;
DeviceState deviceState;
// BLEManager is now a singleton - initialized in main.cpp
// BLEManager bleManager(deviceState, GoToPattern);
