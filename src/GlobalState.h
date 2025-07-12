#pragma once

#include "Globals.h"
#include "UserPreferences.h"
#include "DeviceState.h"
#include "BLEManager.h"
#include <ArduinoBLE.h>
#define AUTH_PIN "1234"  // Default PIN - change this to your preferred PIN
#define MAX_AUTHORIZED_DEVICES 5
extern String authorizedDevices[MAX_AUTHORIZED_DEVICES];
extern int numAuthorizedDevices;
extern bool isAuthenticated;
extern bool pairingMode;
extern unsigned long pairingModeStartTime;
extern const unsigned long PAIRING_TIMEOUT; // 30 seconds

// Brightness pulsing variables
extern bool isPulsing;
extern int pulseTargetBrightness;
extern int previousBrightness;
extern unsigned long pulseStartTime;
extern unsigned long pulseDuration;
extern bool isFadeMode;  // true for fade, false for pulse
extern PreferencesManager prefsManager;
extern DeviceState deviceState;
extern BLEManager bleManager;

// Add stuff for device info, like firmware version, model
/*
export type DeviceTypeInfo = {
  model: string;
  firmwareVersion: string;
  numLEDs: number;
  ledLayout: 'strip' | 'matrix' | 'custom';
  capabilities: string[];
};*/

// Auth characteristic (always available)
extern BLEStringCharacteristic authCharacteristic;
