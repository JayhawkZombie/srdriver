#include "BLEManager.h"
#include "utility/strings.hpp"
#include "BLEUtils.hpp"
#include "Utils.hpp"
#include "WavePlayer.h"
#include <algorithm>
#include "SDCardAPI.h"
extern SDCardAPI sdCardAPI;

// Forward declarations for functions called from handlers
extern void GoToPattern(int patternIndex);
extern WavePlayer* GetCurrentWavePlayer();
extern void UpdateColorFromCharacteristic(BLEStringCharacteristic& characteristic, Light& color, bool isHighColor);
extern void UpdateSeriesCoefficientsFromCharacteristic(BLEStringCharacteristic& characteristic, WavePlayer& wp);
extern void ParseAndExecuteCommand(const String& command);

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

BLEManager::BLEManager(DeviceState& state, std::function<void(int)> goToPatternCb)
    : deviceState(state),
      onSettingChanged(nullptr),
      goToPatternCallback(goToPatternCb),
      controlService("b1862b70-e0ce-4b1b-9734-d7629eb8d711"),
      brightnessCharacteristic("4df3a1f9-2a42-43ee-ac96-f7db09abb4f0", BLERead | BLEWrite | BLENotify, 3),
      speedCharacteristic("a5fb3bc5-9633-4b85-8a42-7756f11ef7ac", BLERead | BLEWrite | BLENotify, 3),
      patternIndexCharacteristic("e95785e0-220e-4cd9-8839-7e92595e47b0", BLERead | BLEWrite | BLENotify, 4),
      highColorCharacteristic("932334a3-8544-4edc-ba49-15055eb1c877", BLERead | BLEWrite | BLENotify, 20),
      lowColorCharacteristic("8cdb8d7f-d2aa-4621-a91f-ca3f54731950", BLERead | BLEWrite | BLENotify, 20),
      leftSeriesCoefficientsCharacteristic("762ff1a5-8965-4d5c-b98e-4faf9b382267", BLERead | BLEWrite | BLENotify, 20),
      rightSeriesCoefficientsCharacteristic("386e0c80-fb59-4e8b-b5d7-6eca4d68ce33", BLERead | BLEWrite | BLENotify, 20),
      commandCharacteristic("c1862b70-e0ce-4b1b-9734-d7629eb8d712", BLERead | BLEWrite | BLENotify, 50),
      heartbeatCharacteristic("f6f7b0f1-c4ab-4c75-9ca7-b43972152f16", BLERead | BLENotify),
      sdCardCommandCharacteristic("89fdb60e-48f3-4bb1-8093-39162054423e", BLERead | BLEWrite | BLENotify, 256),
      sdCardStreamCharacteristic("7b7e6311-de69-49b3-8a27-ac57b9aa2673", BLERead | BLENotify, 512),
      brightnessDescriptor("2901", "Brightness Control"),
      speedDescriptor("2901", "Speed Control"),
      patternIndexDescriptor("2901", "Pattern Index"),
      highColorDescriptor("2901", "High Color"),
      lowColorDescriptor("2901", "Low Color"),
      leftSeriesCoefficientsDescriptor("2901", "Left Series Coefficients"),
      rightSeriesCoefficientsDescriptor("2901", "Right Series Coefficients"),
      commandDescriptor("2901", "Command Interface"),
      heartbeatDescriptor("2901", "Heartbeat"),
      sdCardCommandDescriptor("2901", "SD Card Command"),
      sdCardStreamDescriptor("2901", "SD Card Stream"),
      brightnessFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      speedFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      patternIndexFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      highColorFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      lowColorFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      leftSeriesCoefficientsFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      rightSeriesCoefficientsFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      commandFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      heartbeatFormatDescriptor("2904", (uint8_t *)&ulongFormat, sizeof(BLE2904_Data)),
      sdCardCommandFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data)),
      sdCardStreamFormatDescriptor("2904", (uint8_t *)&stringFormat, sizeof(BLE2904_Data))
{}

void BLEManager::begin() {
    // Add all characteristics to the service
    controlService.addCharacteristic(brightnessCharacteristic);
    controlService.addCharacteristic(speedCharacteristic);
    controlService.addCharacteristic(patternIndexCharacteristic);
    controlService.addCharacteristic(highColorCharacteristic);
    controlService.addCharacteristic(lowColorCharacteristic);
    controlService.addCharacteristic(leftSeriesCoefficientsCharacteristic);
    controlService.addCharacteristic(rightSeriesCoefficientsCharacteristic);
    controlService.addCharacteristic(commandCharacteristic);
    controlService.addCharacteristic(heartbeatCharacteristic);
    controlService.addCharacteristic(sdCardCommandCharacteristic);
    controlService.addCharacteristic(sdCardStreamCharacteristic);
    
    Serial.println("[BLE Manager] Added SD Card Stream characteristic to service");

    // Add descriptors
    brightnessCharacteristic.addDescriptor(brightnessDescriptor);
    speedCharacteristic.addDescriptor(speedDescriptor);
    patternIndexCharacteristic.addDescriptor(patternIndexDescriptor);
    highColorCharacteristic.addDescriptor(highColorDescriptor);
    lowColorCharacteristic.addDescriptor(lowColorDescriptor);
    leftSeriesCoefficientsCharacteristic.addDescriptor(leftSeriesCoefficientsDescriptor);
    rightSeriesCoefficientsCharacteristic.addDescriptor(rightSeriesCoefficientsDescriptor);
    commandCharacteristic.addDescriptor(commandDescriptor);
    heartbeatCharacteristic.addDescriptor(heartbeatDescriptor);
    sdCardCommandCharacteristic.addDescriptor(sdCardCommandDescriptor);
    sdCardStreamCharacteristic.addDescriptor(sdCardStreamDescriptor);

    // Add format descriptors
    brightnessCharacteristic.addDescriptor(brightnessFormatDescriptor);
    speedCharacteristic.addDescriptor(speedFormatDescriptor);
    patternIndexCharacteristic.addDescriptor(patternIndexFormatDescriptor);
    highColorCharacteristic.addDescriptor(highColorFormatDescriptor);
    lowColorCharacteristic.addDescriptor(lowColorFormatDescriptor);
    leftSeriesCoefficientsCharacteristic.addDescriptor(leftSeriesCoefficientsFormatDescriptor);
    rightSeriesCoefficientsCharacteristic.addDescriptor(rightSeriesCoefficientsFormatDescriptor);
    commandCharacteristic.addDescriptor(commandFormatDescriptor);
    heartbeatCharacteristic.addDescriptor(heartbeatFormatDescriptor);
    sdCardCommandCharacteristic.addDescriptor(sdCardCommandFormatDescriptor);
    sdCardStreamCharacteristic.addDescriptor(sdCardStreamFormatDescriptor);

    BLE.addService(controlService);
    BLE.setAdvertisedService(controlService);
    BLE.advertise();

    // Register handlers for writable characteristics
    handlers.push_back({
        &brightnessCharacteristic,
        [this](const unsigned char* value) {
            char buf[16];
            size_t len = std::min(sizeof(buf) - 1, (size_t)brightnessCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String s(buf);
            int rawVal = s.toInt();
            Serial.print("[BLE Manager] Raw brightness value: ");
            Serial.println(rawVal);
            float mapped = getVaryingCurveMappedValue(rawVal / 255.0f, 3.f);
            int mappedVal = static_cast<int>(mapped * 255.0f + 0.5f);
            Serial.print("[BLE Manager] Brightness mapped: ");
            Serial.println(mappedVal);
            deviceState.brightness = mappedVal;
            brightnessCharacteristic.writeValue(String(mappedVal).c_str());
            if (onSettingChanged) onSettingChanged(deviceState);
        }
    });
    handlers.push_back({
        &speedCharacteristic,
        [this](const unsigned char* value) {
            char buf[16];
            size_t len = std::min(sizeof(buf) - 1, (size_t)speedCharacteristic.valueLength());
            memcpy(buf, value, len);
            buf[len] = '\0';
            String s(buf);
            float speed = s.toFloat() / 255.f * 20.f;
            deviceState.speedMultiplier = speed;
            
            // Also update the global speedMultiplier used by PatternManager
            extern float speedMultiplier;
            speedMultiplier = speed;
            
            Serial.print("[BLE Manager] Speed multiplier set to: ");
            Serial.println(speed);
            speedCharacteristic.writeValue(String(speed, 3).c_str());
            if (onSettingChanged) onSettingChanged(deviceState);
        }
    });
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
            char buf[64];
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
        Serial.println("[BLE Manager] Central connected!");
        // Optionally: reset state, send initial values, etc.
    } else if (!connected && wasConnected) {
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
    if (sdCardCommandCharacteristic && sdCardCommandCharacteristic.written()) {
        String command = sdCardCommandCharacteristic.value();
        Serial.print("[BLE Manager] SD Card command received: ");
        Serial.println(command);
        
        sdCardAPI.handleCommand(command);
        String result = sdCardAPI.getLastResult();
        
        Serial.print("[BLE Manager] SD Card result length: ");
        Serial.println(result.length());
        Serial.print("[BLE Manager] SD Card result preview: ");
        Serial.println(result.substring(0, min(100, (int)result.length())));
        
        // Send a small acknowledgment via the command characteristic
        sdCardCommandCharacteristic.setValue("Streaming response...");
        
        // Only stream the result if it's a LIST (or other single-response command)
        if (command.startsWith("LIST")) {
            Serial.println("[BLE Manager] Using streaming for SD card LIST response");
            startStreaming(result, "FILE_LIST");
        }
        // For PRINT, SDCardAPI now calls startStreaming directly
    }

    // Stream next chunk if active (for both LIST and PRINT)
    if (jsonStreamer.isActive()) {
        jsonStreamer.update([&](const String& chunk) {
            Serial.print("[BLE Manager] [STREAM] Sending chunk: ");
            Serial.println(chunk);
            sdCardStreamCharacteristic.writeValue(chunk.c_str());
        });
    }
}

void BLEManager::setOnSettingChanged(OnSettingChangedCallback cb) {
    onSettingChanged = cb;
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
    if (brightnessCharacteristic) {
        brightnessCharacteristic.writeValue(String(deviceState.brightness).c_str());
    }
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
        sdCardStreamCharacteristic.writeValue(data.c_str());
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
            sdCardStreamCharacteristic.writeValue(chunk.c_str());
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
    sdCardStreamCharacteristic.writeValue(envelope.c_str());
}

void BLEManager::handleEvents() {
    for (auto& handler : handlers) {
        if (handler.characteristic && handler.characteristic->written()) {
            Serial.print("[BLE Manager] Characteristic written: ");
            handler.onWrite(handler.characteristic->value());
        }
    }
}

void BLEManager::updateAllCharacteristics() {
    // TODO: Write current deviceState values to BLE characteristics
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
