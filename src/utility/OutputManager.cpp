#include "OutputManager.h"
#include "StringUtils.h"
#include <ArduinoJson.h>
#include <SD.h>

OutputManager& OutputManager::getInstance() {
    static OutputManager instance;
    return instance;
}

void OutputManager::setOutputTarget(OutputTarget target) {
    currentTarget = target;
}

OutputTarget OutputManager::getOutputTarget() const {
    return currentTarget;
}

void OutputManager::print(const String& message) {
    switch (currentTarget) {
        case OutputTarget::SERIAL_OUTPUT:
            printToSerial(message);
            break;
        case OutputTarget::BLE:
            printToBLE(message);
            break;
        case OutputTarget::BOTH:
            printToSerial(message);
            printToBLE(message);
            break;
    }
}

void OutputManager::println(const String& message) {
    switch (currentTarget) {
        case OutputTarget::SERIAL_OUTPUT:
            printlnToSerial(message);
            break;
        case OutputTarget::BLE:
            printToBLE(message + "\n");
            break;
        case OutputTarget::BOTH:
            printlnToSerial(message);
            printToBLE(message + "\n");
            break;
    }
}

void OutputManager::printFile(const String& filename, const String& base64Content) {
    switch (currentTarget) {
        case OutputTarget::SERIAL_OUTPUT:
            printBase64ToSerial(filename, base64Content);
            break;
        case OutputTarget::BLE:
            streamToBLE(base64Content, "PRINT");
            break;
        case OutputTarget::BOTH:
            printBase64ToSerial(filename, base64Content);
            streamToBLE(base64Content, "PRINT");
            break;
    }
}

void OutputManager::printJson(const String& json) {
    switch (currentTarget) {
        case OutputTarget::SERIAL_OUTPUT:
            printJsonToSerial(json);
            break;
        case OutputTarget::BLE:
            streamToBLE(json, "JSON");
            break;
        case OutputTarget::BOTH:
            printJsonToSerial(json);
            streamToBLE(json, "JSON");
            break;
    }
}

void OutputManager::printDirectoryListing(const String& dir, const String& jsonListing) {
    switch (currentTarget) {
        case OutputTarget::SERIAL_OUTPUT:
            printDirectoryListingToSerial(dir, jsonListing);
            break;
        case OutputTarget::BLE:
            streamToBLE(jsonListing, "FILE_LIST");
            break;
        case OutputTarget::BOTH:
            printDirectoryListingToSerial(dir, jsonListing);
            streamToBLE(jsonListing, "FILE_LIST");
            break;
    }
}

void OutputManager::printToSerial(const String& message) {
    Serial.print(message);
}

void OutputManager::printlnToSerial(const String& message) {
    Serial.println(message);
}

void OutputManager::printToBLE(const String& message) {
    // For simple text messages, we could send via a BLE characteristic
    // For now, just print to serial as a fallback
    Serial.print("[BLE] " + message);
}

void OutputManager::streamToBLE(const String& data, const String& type) {
	// Send via BLE if available
#if SUPPORTS_BLE
	// extern BLEManager bleManager; // Remove this since we're using singleton
	BLEManager* ble = BLEManager::getInstance();
	if (ble) {
		ble->startStreaming(data, type);
	}
#endif
}

void OutputManager::printBase64ToSerial(const String& filename, const String& base64Content) {
    Serial.println("=== File: " + filename + " ===");
    
    // Instead of decoding the entire base64 string at once (which can cause memory issues),
    // read the file directly in chunks and print it
    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) {
        Serial.println("Error: Could not open file for reading");
        Serial.println("=== End of file ===");
        return;
    }
    
    const size_t chunkSize = 256; // Read in 256-byte chunks
    uint8_t buffer[chunkSize];
    size_t bytesRead;
    
    while ((bytesRead = file.read(buffer, chunkSize)) > 0) {
        // Print the chunk as text (assuming it's a text file)
        for (size_t i = 0; i < bytesRead; i++) {
            Serial.print((char)buffer[i]);
        }
    }
    
    file.close();
    Serial.println();
    Serial.println("=== End of file ===");
}

void OutputManager::printJsonToSerial(const String& json) {
    // Pretty print JSON to serial
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.println("JSON parse error: " + String(error.c_str()));
        Serial.println("Raw JSON: " + json);
        return;
    }
    
    // Pretty print the JSON
    serializeJsonPretty(doc, Serial);
    Serial.println();
}

void OutputManager::printDirectoryListingToSerial(const String& dir, const String& jsonListing) {
    Serial.println("Directory listing for: " + dir);
    Serial.println("----------------------------------------");
    
    // Parse the JSON to show readable output
    DynamicJsonDocument listDoc(4096);
    DeserializationError error = deserializeJson(listDoc, jsonListing);
    if (!error && listDoc.containsKey("ch")) {
        JsonArray files = listDoc["ch"];
        for (JsonObject file : files) {
            String name = file["f"] | "";
            String type = file["t"] | "";
            String size = "";
            if (type == "f" && file.containsKey("sz")) {
                size = " (" + String(file["sz"].as<long>()) + " bytes)";
            }
            String prefix = (type == "d") ? "[DIR] " : "[FILE]";
            Serial.println(prefix + name + size);
        }
    }
    Serial.println("----------------------------------------");
} 