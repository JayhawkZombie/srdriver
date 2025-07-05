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
