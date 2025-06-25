#include "GlobalState.h"

String authorizedDevices[MAX_AUTHORIZED_DEVICES];
int numAuthorizedDevices = 0;
bool isAuthenticated = false;
bool pairingMode = false;
unsigned long pairingModeStartTime = 0;
const unsigned long PAIRING_TIMEOUT = 30000; // 30 seconds
bool controlServiceAdded = false; // Track if control service has been added

// Create separate services for security
BLEService authService("a1862b70-e0ce-4b1b-9734-d7629eb8d710");  // Auth service (always advertised)
BLEService controlService("b1862b70-e0ce-4b1b-9734-d7629eb8d711"); // Control service (only after auth)
int GlobalBrightness = 155;

// Brightness pulsing variables
bool isPulsing = false;
int pulseTargetBrightness = 0;
int previousBrightness = 0;
unsigned long pulseStartTime = 0;
unsigned long pulseDuration = 0;

// Auth characteristic (always available)
BLEStringCharacteristic authCharacteristic("a1b2c3d4-e5f6-7890-abcd-ef1234567890", BLERead | BLEWrite | BLENotify, 10);

// Control characteristics (only available after authentication)
BLEStringCharacteristic brightnessCharacteristic("4df3a1f9-2a42-43ee-ac96-f7db09abb4f0", BLERead | BLEWrite | BLENotify, 3);
BLEStringCharacteristic speedCharacteristic("a5fb3bc5-9633-4b85-8a42-7756f11ef7ac", BLERead | BLEWrite | BLENotify, 3);
BLEStringCharacteristic patternIndexCharacteristic("e95785e0-220e-4cd9-8839-7e92595e47b0", BLERead | BLEWrite | BLENotify, 4);
BLEStringCharacteristic highColorCharacteristic("932334a3-8544-4edc-ba49-15055eb1c877", BLERead | BLEWrite | BLENotify, 20);
BLEStringCharacteristic lowColorCharacteristic("8cdb8d7f-d2aa-4621-a91f-ca3f54731950", BLERead | BLEWrite | BLENotify, 20);
BLEStringCharacteristic leftSeriesCoefficientsCharacteristic("762ff1a5-8965-4d5c-b98e-4faf9b382267", BLERead | BLEWrite | BLENotify, 20);
BLEStringCharacteristic rightSeriesCoefficientsCharacteristic("386e0c80-fb59-4e8b-b5d7-6eca4d68ce33", BLERead | BLEWrite | BLENotify, 20);
BLEStringCharacteristic commandCharacteristic("c1862b70-e0ce-4b1b-9734-d7629eb8d712", BLERead | BLEWrite | BLENotify, 50);

// BLE Descriptors for human-readable names
BLEDescriptor brightnessDescriptor("2901", "Brightness Control");
BLEDescriptor speedDescriptor("2901", "Speed Control");
BLEDescriptor patternIndexDescriptor("2901", "Pattern Index");
BLEDescriptor highColorDescriptor("2901", "High Color");
BLEDescriptor lowColorDescriptor("2901", "Low Color");
BLEDescriptor leftSeriesCoefficientsDescriptor("2901", "Left Series Coefficients");
BLEDescriptor rightSeriesCoefficientsDescriptor("2901", "Right Series Coefficients");
BLEDescriptor authDescriptor("2901", "Authentication");
BLEDescriptor commandDescriptor("2901", "Command Interface");

// Create format descriptors for UTF-8 strings
BLE2904_Data stringFormat = {
    0x1A,      // m_format: UTF-8 String with null termination
    0,         // m_exponent: No exponent
    0x0000,    // m_unit: No unit
    0x01,      // m_namespace: Bluetooth SIG namespace
    0x0000     // m_description: No description
};

BLEDescriptor brightnessFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor speedFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor patternIndexFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor highColorFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor lowColorFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor leftSeriesCoefficientsFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor rightSeriesCoefficientsFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));
BLEDescriptor commandFormatDescriptor("2904", (uint8_t *) &stringFormat, sizeof(BLE2904_Data));

