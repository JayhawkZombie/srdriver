#pragma once

#include "Globals.h"
#include "UserPreferences.h"
#include "DeviceState.h"
#include "hal/ble/BLEManager.h"
#include <ArduinoBLE.h>

extern bool isBooting;  // true during device startup, false after initialization
extern PreferencesManager prefsManager;
extern DeviceState deviceState;

// Add stuff for device info, like firmware version, model
/*
export type DeviceTypeInfo = {
  model: string;
  firmwareVersion: string;
  numLEDs: number;
  ledLayout: 'strip' | 'matrix' | 'custom';
  capabilities: string[];
};*/
