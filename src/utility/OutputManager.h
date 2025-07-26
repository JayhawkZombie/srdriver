#pragma once

#include <Arduino.h>
#include "hal/ble/BLEManager.h"

enum class OutputTarget {
    SERIAL_OUTPUT,
    BLE,
    BOTH
};

enum class LogMode {
    NORMAL,     // All logging enabled
    INTERACTIVE // Suppress debug/info logging for clean CLI
};

class OutputManager {
public:
    static OutputManager& getInstance();
    
    // Set the current output target (defaults to BLE for backward compatibility)
    void setOutputTarget(OutputTarget target);
    OutputTarget getOutputTarget() const;
    
    // Set logging mode (normal vs interactive)
    void setLogMode(LogMode mode);
    LogMode getLogMode() const;
    
    // Check if we should suppress logging in interactive mode
    bool shouldSuppressLogging() const;
    
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
    
    // Interactive mode helpers
    void enterInteractiveMode();
    void exitInteractiveMode();
    void showInteractivePrompt();
    void handleInteractiveCommand(const String& command);

private:
    OutputManager() = default;
    OutputTarget currentTarget = OutputTarget::BLE;
    LogMode currentLogMode = LogMode::NORMAL;
    bool interactiveMode = false;
    
    // Helper methods
    void printBase64ToSerial(const String& filename, const String& base64Content);
    void printJsonToSerial(const String& json);
    void printDirectoryListingToSerial(const String& dir, const String& jsonListing);
}; 