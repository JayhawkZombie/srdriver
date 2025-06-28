#pragma once

#include "Globals.h"
#include <ArduinoBLE.h>
#define AUTH_PIN "1234"  // Default PIN - change this to your preferred PIN
#define MAX_AUTHORIZED_DEVICES 5
extern String authorizedDevices[MAX_AUTHORIZED_DEVICES];
extern int numAuthorizedDevices;
extern bool isAuthenticated;
extern bool pairingMode;
extern unsigned long pairingModeStartTime;
extern const unsigned long PAIRING_TIMEOUT; // 30 seconds
extern bool controlServiceAdded; // Track if control service has been added

// Create separate services for security
extern BLEService authService;  // Auth service (always advertised)
extern BLEService controlService; // Control service (only after auth)
extern int GlobalBrightness;

// Brightness pulsing variables
extern bool isPulsing;
extern int pulseTargetBrightness;
extern int previousBrightness;
extern unsigned long pulseStartTime;
extern unsigned long pulseDuration;

// Auth characteristic (always available)
extern BLEStringCharacteristic authCharacteristic;

// Control characteristics (only available after authentication)
extern BLEStringCharacteristic brightnessCharacteristic;
extern BLEStringCharacteristic speedCharacteristic;
extern BLEStringCharacteristic patternIndexCharacteristic;
extern BLEStringCharacteristic highColorCharacteristic;
extern BLEStringCharacteristic lowColorCharacteristic;
extern BLEStringCharacteristic leftSeriesCoefficientsCharacteristic;
extern BLEStringCharacteristic rightSeriesCoefficientsCharacteristic;
extern BLEStringCharacteristic commandCharacteristic;

// BLE Descriptors for human-readable names
extern BLEDescriptor brightnessDescriptor;
extern BLEDescriptor speedDescriptor;
extern BLEDescriptor patternIndexDescriptor;
extern BLEDescriptor highColorDescriptor;
extern BLEDescriptor lowColorDescriptor;
extern BLEDescriptor leftSeriesCoefficientsDescriptor;
extern BLEDescriptor rightSeriesCoefficientsDescriptor;
extern BLEDescriptor authDescriptor;
extern BLEDescriptor commandDescriptor;

struct BLE2904_Data {
    uint8_t m_format;
    int8_t m_exponent;
    uint16_t m_unit;
    uint8_t m_namespace;
    uint16_t m_description;
} __attribute__((packed));

extern BLEDescriptor brightnessFormatDescriptor;
extern BLEDescriptor patternIndexFormatDescriptor;
extern BLEDescriptor speedFormatDescriptor;
extern BLEDescriptor highColorFormatDescriptor;
extern BLEDescriptor lowColorFormatDescriptor;
extern BLEDescriptor leftSeriesCoefficientsFormatDescriptor;
extern BLEDescriptor rightSeriesCoefficientsFormatDescriptor;
extern BLEDescriptor commandFormatDescriptor;


