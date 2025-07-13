#pragma once

#include <Arduino.h>
#include "BLEManager.h"

enum class OutputTarget {
    SERIAL_OUTPUT,
    BLE,
    BOTH
};

class OutputManager {
public:
    static OutputManager& getInstance();
    
    // Set the current output target (defaults to BLE for backward compatibility)
    void setOutputTarget(OutputTarget target);
    OutputTarget getOutputTarget() const;
    
    // Print methods that route to the appropriate target
    void print(const String& message);
    void println(const String& message);
    void printFile(const String& filename, const String& base64Content);
    void printJson(const String& json);
    void printDirectoryListing(const String& dir, const String& jsonListing);
    
    // Direct access methods for specific targets
    void printToSerial(const String& message);
    void printlnToSerial(const String& message);
    void printToBLE(const String& message);
    void streamToBLE(const String& data, const String& type);

private:
    OutputManager() = default;
    OutputTarget currentTarget = OutputTarget::BLE;
    
    // Helper methods
    void printBase64ToSerial(const String& filename, const String& base64Content);
    void printJsonToSerial(const String& json);
    void printDirectoryListingToSerial(const String& dir, const String& jsonListing);
}; 