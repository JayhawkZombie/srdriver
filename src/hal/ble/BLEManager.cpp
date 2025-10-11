#include "hal/ble/BLEManager.h"
#include "utility/strings.hpp"
#include "BLEUtils.hpp"
#include "Utils.hpp"
#include "../lights/WavePlayer.h"
#include <algorithm>
#if SUPPORTS_SD_CARD
#include "hal/SDCardAPI.h"
#include "utility/OutputManager.h"
#endif
#include "hal/display/DisplayQueue.h"

// Forward declarations for functions called from handlers
extern void GoToPattern(int patternIndex);
extern WavePlayer* GetCurrentWavePlayer();
extern void UpdateColorFromCharacteristic(BLEStringCharacteristic& characteristic, Light& color, bool isHighColor);
extern void UpdateSeriesCoefficientsFromCharacteristic(BLEStringCharacteristic& characteristic, WavePlayer& wp);
extern void ParseAndExecuteCommand(const String& command);

// Singleton instance
BLEManager* BLEManager::instance = nullptr;

BLE2904_Data BLEManager::stringFormat = {
    0x1A,      // m_format: UTF-8 String with null termination
    0,         // m_exponent: No exponent
    0x0000,    // m_unit: No unit
    0x01,      // m_namespace: Bluetooth SIG namespace
    0x0000     // m_description: No description
};
BLE2904_Data BLEManager::ulongFormat = {
    0x06,      // m_format: uint32 (unsigned 32-bit integer)
    0,         // m_exponent: No exponent
    0x0000,    // m_unit: No unit
    0x01,      // m_namespace: Bluetooth SIG namespace
    0x0000     // m_description: No description
};

// Singleton methods
void BLEManager::initialize(DeviceState& state, std::function<void(int)> goToPatternCb) {
    Serial.println("[BLEManager] initialize() called");
    if (instance == nullptr) {
        Serial.println("[BLEManager] Creating new instance...");
        instance = new BLEManager(state, goToPatternCb);
        Serial.println("[BLEManager] Instance created successfully");
    } else {
        Serial.println("[BLEManager] Instance already exists");
    }
}

void BLEManager::destroy() {
    Serial.println("[BLEManager] destroy() called");
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
        Serial.println("[BLEManager] Instance destroyed");
    }
}

BLEManager::BLEManager(DeviceState& state, std::function<void(int)> goToPatternCb)
    : deviceState(state),
      onSettingChanged(nullptr),
      goToPatternCallback(goToPatternCb),
      controlService("b1862b70-e0ce-4b1b-9734-d7629eb8d711"),
      patternIndexCharacteristic("e95785e0-220e-4cd9-8839-7e92595e47b0", BLERead | BLEWrite | BLENotify, 4),
      highColorCharacteristic("932334a3-8544-4edc-ba49-15055eb1c877", BLERead | BLEWrite | BLENotify, 20),
      lowColorCharacteristic("8cdb8d7f-d2aa-4621-a91f-ca3f54731950", BLERead | BLEWrite | BLENotify, 20),
      leftSeriesCoefficientsCharacteristic("762ff1a5-8965-4d5c-b98e-4faf9b382267", BLERead | BLEWrite | BLENotify, 20),
      rightSeriesCoefficientsCharacteristic("386e0c80-fb59-4e8b-b5d7-6eca4d68ce33", BLERead | BLEWrite | BLENotify, 20),
      commandCharacteristic("c1862b70-e0ce-4b1b-9734-d7629eb8d712", BLERead | BLEWrite | BLENotify, 200),
      ipAddressCharacteristic("a1b2c3d4-e5f6-7890-abcd-ef1234567890", BLERead | BLENotify, 20),
    wifiSSIDCharacteristic("04a1d69b-efbc-4919-9b61-b557bdafeb8a", BLERead | BLEWrite | BLENotify, 32),
    wifiPasswordCharacteristic("21308ad6-e818-41fa-a81f-c5995cc938ac", BLERead | BLEWrite | BLENotify, 64),
    wifiStatusCharacteristic("f3d6b6b2-a507-413f-9d41-952fbe3cc494", BLERead | BLENotify, 20),
      heartbeatCharacteristic("f6f7b0f1-c4ab-4c75-9ca7-b43972152f16", BLERead | BLENotify),
      patternIndexDescriptor("2901", "Pattern Index"),
      highColorDescriptor("2901", "High Color"),
      lowColorDescriptor("2901", "Low Color"),
      leftSeriesCoefficientsDescriptor("2901", "Left Series Coefficients"),
      rightSeriesCoefficientsDescriptor("2901", "Right Series Coefficients"),
      commandDescriptor("2901", "Command Interface"),
      ipAddressDescriptor("2901", "IP Address"),
      wifiSSIDDescriptor("2901", "WiFi SSID"),
      wifiPasswordDescriptor("2901", "WiFi Password"),
      wifiStatusDescriptor("2901", "WiFi Status"),
      heartbeatDescriptor("2901", "Heartbeat"),
      patternIndexFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      highColorFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      lowColorFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      leftSeriesCoefficientsFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      rightSeriesCoefficientsFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      commandFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      ipAddressFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      wifiSSIDFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      wifiPasswordFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      wifiStatusFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      heartbeatFormatDescriptor("2904", (uint8_t *)&ulongFormat, sizeof(BLE2904_Data)),
      registry(&controlService)
#if SUPPORTS_SD_CARD
      ,sdCardCommandFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data))
      ,sdCardStreamFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data))
      ,sdCardCommandCharacteristic("89fdb60e-48f3-4bb1-8093-39162054423e", BLERead | BLEWrite | BLENotify, 256)
      ,sdCardStreamCharacteristic("7b7e6311-de69-49b3-8a27-ac57b9aa2673", BLERead | BLENotify, 512)
      ,sdCardCommandDescriptor("2901", "SD Card Command")
      ,sdCardStreamDescriptor("2901", "SD Card Stream")
#endif
{}

void BLEManager::begin() {
    // Register any additional characteristics first
    registerCharacteristics();
    
    // Set a fake IP address for testing
    setIPAddress("192.168.1.100");
    
    // Initialize WiFi status
    setWiFiStatus("disconnected");
    Serial.println("[BLE Manager] WiFi characteristics initialized");
    
    // Add the service to BLE and start advertising
    BLE.addService(controlService);
    BLE.setAdvertisedService(controlService);
    BLE.advertise();
}

void BLEManager::registerCharacteristics() {
    // Add all characteristics to the service
    // Speed is now handled by SpeedController via registry
    controlService.addCharacteristic(patternIndexCharacteristic);
    controlService.addCharacteristic(highColorCharacteristic);
    controlService.addCharacteristic(lowColorCharacteristic);
    controlService.addCharacteristic(leftSeriesCoefficientsCharacteristic);
    controlService.addCharacteristic(rightSeriesCoefficientsCharacteristic);
    controlService.addCharacteristic(commandCharacteristic);
    controlService.addCharacteristic(ipAddressCharacteristic);
    controlService.addCharacteristic(wifiSSIDCharacteristic);
    controlService.addCharacteristic(wifiPasswordCharacteristic);
    controlService.addCharacteristic(wifiStatusCharacteristic);
    controlService.addCharacteristic(heartbeatCharacteristic);
#if SUPPORTS_SD_CARD
    controlService.addCharacteristic(sdCardCommandCharacteristic);
    controlService.addCharacteristic(sdCardStreamCharacteristic);
#endif
    
    Serial.println("[BLE Manager] Added SD Card Stream characteristic to service");

    // Add descriptors
    // Speed descriptors are now handled by SpeedController via registry
    patternIndexCharacteristic.addDescriptor(patternIndexDescriptor);
    highColorCharacteristic.addDescriptor(highColorDescriptor);
    lowColorCharacteristic.addDescriptor(lowColorDescriptor);
    leftSeriesCoefficientsCharacteristic.addDescriptor(leftSeriesCoefficientsDescriptor);
    rightSeriesCoefficientsCharacteristic.addDescriptor(rightSeriesCoefficientsDescriptor);
    commandCharacteristic.addDescriptor(commandDescriptor);
    ipAddressCharacteristic.addDescriptor(ipAddressDescriptor);
    wifiSSIDCharacteristic.addDescriptor(wifiSSIDDescriptor);
    wifiPasswordCharacteristic.addDescriptor(wifiPasswordDescriptor);
    wifiStatusCharacteristic.addDescriptor(wifiStatusDescriptor);
    heartbeatCharacteristic.addDescriptor(heartbeatDescriptor);
#if SUPPORTS_SD_CARD
    sdCardCommandCharacteristic.addDescriptor(sdCardCommandDescriptor);
    sdCardStreamCharacteristic.addDescriptor(sdCardStreamDescriptor);
#endif

    // Add format descriptors
    // Speed format descriptor is now handled by SpeedController via registry
    patternIndexCharacteristic.addDescriptor(patternIndexFormatDescriptor);
    highColorCharacteristic.addDescriptor(highColorFormatDescriptor);
    lowColorCharacteristic.addDescriptor(lowColorFormatDescriptor);
    leftSeriesCoefficientsCharacteristic.addDescriptor(leftSeriesCoefficientsFormatDescriptor);
    rightSeriesCoefficientsCharacteristic.addDescriptor(rightSeriesCoefficientsFormatDescriptor);
    commandCharacteristic.addDescriptor(commandFormatDescriptor);
    ipAddressCharacteristic.addDescriptor(ipAddressFormatDescriptor);
    wifiSSIDCharacteristic.addDescriptor(wifiSSIDFormatDescriptor);
    wifiPasswordCharacteristic.addDescriptor(wifiPasswordFormatDescriptor);
    wifiStatusCharacteristic.addDescriptor(wifiStatusFormatDescriptor);
    heartbeatCharacteristic.addDescriptor(heartbeatFormatDescriptor);
#if SUPPORTS_SD_CARD
    sdCardCommandCharacteristic.addDescriptor(sdCardCommandFormatDescriptor);
    sdCardStreamCharacteristic.addDescriptor(sdCardStreamFormatDescriptor);
#endif

    // Register handlers for writable characteristics
    // Speed is now handled by SpeedController via registry
    handlers.push_back({
        &patternIndexCharacteristic,
        [this](const unsigned char* value) {
            char buf[16];
            size_t len = std::min(sizeof(buf) - 1, (size_t)patternIndexCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String s(buf);
            int val = s.toInt();
            Serial.print("[BLE Manager] Pattern index set to: ");
            Serial.println(val);
            patternIndexCharacteristic.writeValue(String(val).c_str());
            GoToPattern(val);
            deviceState.patternIndex = val;
            if (onSettingChanged) onSettingChanged(deviceState);
        }
    });
    handlers.push_back({
        &highColorCharacteristic,
        [this](const unsigned char* value) {
            char buf[32];
            size_t len = std::min(sizeof(buf) - 1, (size_t)highColorCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String s(buf);
            Serial.print("[BLE Manager] High color set to: ");
            Serial.println(s);
            // deviceState.highColor = ParseColor(s);
            highColorCharacteristic.writeValue(s.c_str());
            
            WavePlayer* currentWavePlayer = GetCurrentWavePlayer();
            if (currentWavePlayer) {
                UpdateColorFromCharacteristic(highColorCharacteristic, currentWavePlayer->hiLt, true);
            }
            
            if (onSettingChanged) onSettingChanged(deviceState);
        }
    });
    handlers.push_back({
        &lowColorCharacteristic,
        [this](const unsigned char* value) {
            char buf[32];
            size_t len = std::min(sizeof(buf) - 1, (size_t)lowColorCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String s(buf);
            Serial.print("[BLE Manager] Low color set to: ");
            Serial.println(s);
            lowColorCharacteristic.writeValue(s.c_str());
            
            WavePlayer* currentWavePlayer = GetCurrentWavePlayer();
            if (currentWavePlayer) {
                UpdateColorFromCharacteristic(lowColorCharacteristic, currentWavePlayer->loLt, false);
            }
            
            if (onSettingChanged) onSettingChanged(deviceState);
        }
    });
    handlers.push_back({
        &leftSeriesCoefficientsCharacteristic,
        [this](const unsigned char* value) {
            char buf[64];
            size_t len = std::min(sizeof(buf) - 1, (size_t)leftSeriesCoefficientsCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String s(buf);
            Serial.print("[BLE Manager] Left series coefficients set to: ");
            Serial.println(s);
            leftSeriesCoefficientsCharacteristic.writeValue(s.c_str());
            
            WavePlayer* currentWavePlayer = GetCurrentWavePlayer();
            if (currentWavePlayer) {
                Serial.println("[BLE Manager] Updating left series coefficients for current wave player");
                UpdateSeriesCoefficientsFromCharacteristic(leftSeriesCoefficientsCharacteristic, *currentWavePlayer);
            } else {
                Serial.println("[BLE Manager] No wave player available for series coefficients update");
            }
            
            if (onSettingChanged) onSettingChanged(deviceState);
        }
    });
    handlers.push_back({
        &rightSeriesCoefficientsCharacteristic,
        [this](const unsigned char* value) {
            char buf[64];
            size_t len = std::min(sizeof(buf) - 1, (size_t)rightSeriesCoefficientsCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String s(buf);
            Serial.print("[BLE Manager] Right series coefficients set to: ");
            Serial.println(s);
            rightSeriesCoefficientsCharacteristic.writeValue(s.c_str());
            
            WavePlayer* currentWavePlayer = GetCurrentWavePlayer();
            if (currentWavePlayer) {
                Serial.println("[BLE Manager] Updating right series coefficients for current wave player");
                UpdateSeriesCoefficientsFromCharacteristic(rightSeriesCoefficientsCharacteristic, *currentWavePlayer);
            } else {
                Serial.println("[BLE Manager] No wave player available for series coefficients update");
            }
            
            if (onSettingChanged) onSettingChanged(deviceState);
        }
    });
    handlers.push_back({
        &commandCharacteristic,
        [this](const unsigned char* value) {
            char buf[256];
            size_t len = std::min(sizeof(buf) - 1, (size_t)commandCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String s(buf);
            Serial.print("[BLE Manager] Command received: ");
            Serial.println(s);
            commandCharacteristic.writeValue(s.c_str());
            ParseAndExecuteCommand(s);
        }
    });
    
    // WiFi SSID handler
    handlers.push_back({
        &wifiSSIDCharacteristic,
        [this](const unsigned char* value) {
            char buf[64];
            size_t len = std::min(sizeof(buf) - 1, (size_t)wifiSSIDCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String ssid(buf);
            Serial.println("[BLE Manager] WiFi SSID received: " + ssid);
            Serial.println("[BLE Manager] WiFi SSID handler triggered!");
            // TODO: Store in preferences and trigger WiFi connection
        }
    });
    
    // WiFi Password handler
    handlers.push_back({
        &wifiPasswordCharacteristic,
        [this](const unsigned char* value) {
            char buf[128];
            size_t len = std::min(sizeof(buf) - 1, (size_t)wifiPasswordCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String password(buf);
            Serial.println("[BLE Manager] WiFi Password received: " + String(password.length()) + " characters");
            Serial.println("[BLE Manager] WiFi Password handler triggered!");
            // TODO: Store in preferences and trigger WiFi connection
        }
    });
}

void BLEManager::startStreaming(const String& json, const String& type) {
    jsonStreamer.begin(json, type);
}

void BLEManager::update() {
    // Handle BLE events and written characteristics
    handleEvents();

    // Connection management
    static bool wasConnected = false;
    bool connected = BLE.connected();
    if (connected && !wasConnected) {
        DisplayQueue::getInstance().setMessageTimeout(4000);
        DisplayQueue::getInstance().safeRequestBannerMessage(DisplayQueue::TASK_BLE, "Connected");
        Serial.println("[BLE Manager] Central connected!");
        // Optionally: reset state, send initial values, etc.
    } else if (!connected && wasConnected) {
        DisplayQueue::getInstance().setMessageTimeout(4000);
        DisplayQueue::getInstance().safeRequestBannerMessage(DisplayQueue::TASK_BLE, "Disconnected");
        Serial.println("[BLE Manager] Central disconnected!");
        // Optionally: reset state, stop streaming, etc.
    }
    wasConnected = connected;

    // Heartbeat or periodic updates
    static unsigned long lastHeartbeat = 0;
    unsigned long now = millis();
    if (connected && now - lastHeartbeat > 5000) {
        heartbeatCharacteristic.writeValue(now);
        lastHeartbeat = now;
    }

    // Any other periodic BLE-related logic
#if SUPPORTS_SD_CARD
    if (sdCardCommandCharacteristic && sdCardCommandCharacteristic.written()) {
        String command = sdCardCommandCharacteristic.value();
        Serial.print("[BLE Manager] SD Card command received: ");
        Serial.println(command);
        
        // Set output target to BLE for commands received via BLE
        SDCardAPI::getInstance().setOutputTarget(OutputTarget::BLE);
        SDCardAPI::getInstance().handleCommand(command);
        
        // Send a small acknowledgment via the command characteristic
        sdCardCommandCharacteristic.setValue("Command processed");
    }
#endif

    // Stream next chunk if active (for both LIST and PRINT)
    if (jsonStreamer.isActive()) {
        jsonStreamer.update([&](const String& chunk) {
            Serial.print("[BLE Manager] [STREAM] Sending chunk: ");
            Serial.println(chunk);
#if SUPPORTS_SD_CARD
            sdCardStreamCharacteristic.writeValue(chunk.c_str());
#endif
        });
    }
}

void BLEManager::setOnSettingChanged(OnSettingChangedCallback cb) {
    onSettingChanged = cb;
}

void BLEManager::triggerOnSettingChanged() {
    if (onSettingChanged) {
        onSettingChanged(deviceState);
    }
}

// void BLEManager::setupCharacteristics() {
//     // brightnessCharacteristic is already set up in the constructor
//     BLEService sdCardService("a05389e7-5efe-46cd-a980-3f8a9763e00a");
//     sdCardCommandCharacteristic = new BLEStringCharacteristic(
//         "89fdb60e-48f3-4bb1-8093-39162054423e",
//         BLEWrite | BLENotify,
//         256
//     );
//     sdCardService.addCharacteristic(*sdCardCommandCharacteristic);
//     BLE.addService(sdCardService);
// }

void BLEManager::updateBrightness() {
    // This function is no longer needed as brightness is managed by BrightnessController
    // if (brightnessCharacteristic) {
    //     brightnessCharacteristic.writeValue(String(deviceState.brightness).c_str());
    // }
}

void BLEManager::streamData(const String& data) {
    if (!BLE.connected()) {
        Serial.println("[BLE Manager] Not connected, cannot stream data");
        return;
    }

    Serial.print("[BLE Manager] Streaming ");
    Serial.print(data.length());
    Serial.println(" bytes of data");
    
    // Check if data is too large for the characteristic (max 512 bytes)
    const int maxChunkSize = 500; // Leave some room for safety
    int dataLength = data.length();
    
    if (dataLength <= maxChunkSize) {
        Serial.println("[BLE Manager] Sending data in single chunk");
        Serial.print("[BLE Manager] Chunk content: ");
        Serial.println(data);
#if SUPPORTS_SD_CARD
        sdCardStreamCharacteristic.writeValue(data.c_str());
#endif
    } else {
        Serial.print("[BLE Manager] Data too large, chunking into ");
        Serial.print((dataLength + maxChunkSize - 1) / maxChunkSize);
        Serial.println(" chunks");
        for (int i = 0; i < dataLength; i += maxChunkSize) {
            String chunk = data.substring(i, i + maxChunkSize);
            Serial.print("[BLE Manager] Sending chunk ");
            Serial.print(i / maxChunkSize + 1);
            Serial.print(" (");
            Serial.print(chunk.length());
            Serial.println(" bytes)");
            Serial.print("[BLE Manager] Chunk content: ");
            Serial.println(chunk);
#if SUPPORTS_SD_CARD
            sdCardStreamCharacteristic.writeValue(chunk.c_str());
#endif
            // Small delay between chunks to prevent overwhelming BLE
            delay(10);
        }
    }
    Serial.println("[BLE Manager] Stream complete");
}

void BLEManager::sendFileDataChunk(const String& envelope) {
    Serial.print("[BLE Manager] [PRINT] About to send file data chunk: ");
    Serial.println(envelope);
    Serial.print("[BLE Manager] [PRINT] Envelope length: ");
    Serial.println(envelope.length());
#if SUPPORTS_SD_CARD
    sdCardStreamCharacteristic.writeValue(envelope.c_str());
#endif
}

void BLEManager::handleEvents() {
    // Handle old-style handlers
    for (auto& handler : handlers) {
        if (handler.characteristic && handler.characteristic->written()) {
            Serial.print("[BLE Manager] Characteristic written: ");
            handler.onWrite(handler.characteristic->value());
        }
    }
    
    // Handle registry characteristics
    const auto& registryChars = registry.getCharacteristics();
    for (auto& info : registryChars) {
        if (info.characteristic) {
           if (info.characteristic->written()) {
                Serial.print("[BLE Manager] Registry characteristic written: ");
                Serial.println(info.name);
                if (info.onWrite) {
                    info.onWrite(info.characteristic->value(), info.characteristic->valueLength());
                } else {
                    Serial.println("[BLE Manager] No onWrite handler for characteristic");
                }
            }
        } else {
            Serial.print("[BLE Manager] Characteristic is null: ");
            Serial.println(info.name);
        }
    }
}

void BLEManager::updateAllCharacteristics() {
    // TODO: Write current deviceState values to BLE characteristics
}

void BLEManager::setIPAddress(const String& ipAddress) {
    Serial.println("[BLE Manager] Setting IP address: " + ipAddress);
    ipAddressCharacteristic.writeValue(ipAddress.c_str());
}

void BLEManager::setWiFiStatus(const String& status) {
    Serial.println("[BLE Manager] Setting WiFi status: " + status);
    wifiStatusCharacteristic.writeValue(status.c_str());
}

String BLEManager::getWiFiStatus() {
    return String(wifiStatusCharacteristic.value());
}

void BLEManager::updateCharacteristic(BLECharacteristic& characteristic, int value) {
    char buf[64];
    safeIntToString(value, buf);
    characteristic.writeValue(buf);
}

void BLEManager::updateCharacteristic(BLECharacteristic& characteristic, const Light& color) {
    char buf[64];
    safeLightToString(color, buf);
    characteristic.writeValue(buf);
}

// BLECharacteristicRegistry implementation
void BLECharacteristicRegistry::registerCharacteristic(BLECharacteristicInfo& info) {
    Serial.print("[BLE Registry] Registering characteristic: ");
    Serial.println(info.name);
    
    // Create BLE objects
    createBLEObjects(info);
    
    // Add to service
    addToService(info);
    
    // Store in registry
    characteristics.push_back(info);
    
    Serial.print("[BLE Registry] Successfully registered: ");
    Serial.println(info.name);
}

void BLECharacteristicRegistry::unregisterCharacteristic(const String& uuid) {
    for (auto it = characteristics.begin(); it != characteristics.end(); ++it) {
        if (it->characteristicUuid == uuid) {
            Serial.print("[BLE Registry] Unregistering characteristic: ");
            Serial.println(it->name);
            characteristics.erase(it);
            break;
        }
    }
}

void BLECharacteristicRegistry::updateAllCharacteristics() {
    for (auto& info : characteristics) {
        if (info.onRead && info.characteristic) {
            String value = info.onRead();
            info.characteristic->writeValue(value.c_str());
        }
    }
}

void BLECharacteristicRegistry::handleCharacteristicWrite(const String& uuid, const unsigned char* value, size_t length) {
    for (auto& info : characteristics) {
        if (info.characteristicUuid == uuid && info.onWrite) {
            info.onWrite(value, length);
            break;
        }
    }
}

void BLECharacteristicRegistry::createBLEObjects(BLECharacteristicInfo& info) {
    // Determine characteristic properties
    int properties = 0;
    if (info.isReadable) properties |= BLERead;
    if (info.isWritable) properties |= BLEWrite;
    if (info.isNotifiable) properties |= BLENotify;
    
    // Create characteristic based on format
    if (info.formatData.m_format == 0x1A) { // String format
        info.characteristic = new BLEStringCharacteristic(
            info.characteristicUuid.c_str(),
            properties,
            info.maxValueLength
        );
    } else if (info.formatData.m_format == 0x06) { // Unsigned long format
        info.characteristic = new BLEUnsignedLongCharacteristic(
            info.characteristicUuid.c_str(),
            properties
        );
    } else {
        // Default to string characteristic
        info.characteristic = new BLEStringCharacteristic(
            info.characteristicUuid.c_str(),
            properties,
            info.maxValueLength
        );
    }
    
    // Create descriptors
    info.descriptor = new BLEDescriptor(
        info.descriptorUuid.c_str(),
        info.name.c_str()
    );
    
    info.formatDescriptor = new BLEDescriptor(
        info.formatDescriptorUuid.c_str(),
        (uint8_t*)&info.formatData,
        sizeof(BLE2904_Data)
    );
}

void BLECharacteristicRegistry::addToService(BLECharacteristicInfo& info) {
    if (service && info.characteristic) {
        service->addCharacteristic(*info.characteristic);
        
        if (info.descriptor) {
            info.characteristic->addDescriptor(*info.descriptor);
        }
        
        if (info.formatDescriptor) {
            info.characteristic->addDescriptor(*info.formatDescriptor);
        }
    }
}
