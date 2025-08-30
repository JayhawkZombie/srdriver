#pragma once

#include <Arduino.h>
#include <vector>
#include "version.h"  // Auto-generated version information

class DeviceInfo {
public:
    DeviceInfo() = default;
    ~DeviceInfo() = default;

    static String GetCompiledFirmwareVersion() {
        return String(FIRMWARE_VERSION);
    }

    // Getter methods for version information
    String getFirmwareVersion() const { return _firmwareVersion; }
    String getDeviceVersion() const { return _deviceVersion; }
    String getDeviceName() const { return _deviceName; }
    String getBuildDate() const { return _buildDate; }
    String getBuildTime() const { return _buildTime; }
    String getBuildTimestamp() const { return _buildTimestamp; }

    // Get all capabilities
    const std::vector<String>& getCapabilities() const { return _capabilities; }

    // Add a capability
    void addCapability(const String& capability) {
        _capabilities.push_back(capability);
    }

private:
    // Device name, should be loaded from settings
    String _deviceName = "SRDriver";

    // Firmware version from generated version.h file
    String _firmwareVersion = FIRMWARE_VERSION;
    String _buildDate = BUILD_DATE;
    String _buildTime = BUILD_TIME;
    String _buildTimestamp = BUILD_TIMESTAMP;

    // Will include info about the hardware revision
    // Eg: srd001-sd1-tp1-dp1-ld1 means SRDriver revision 001, sd card rev 1, temp rev 1, display rev1, led rev1
    //     srd001-sd1-tp1-dp1-ld2 would indicate a later display revision
    String _deviceVersion = "";

    std::vector<String> _capabilities;    

};
