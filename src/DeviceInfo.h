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
    static String getFirmwareVersion() { return _firmwareVersion; }
    static String getDeviceVersion() { return _deviceVersion; }
    static String getDeviceName() { return _deviceName; }
    static String getBuildDate() { return _buildDate; }
    static String getBuildTime() { return _buildTime; }
    static String getBuildTimestamp() { return _buildTimestamp; }
    static String getVersionBranch() { return _versionBranch; }

    // Get all capabilities
    const std::vector<String>& getCapabilities() const { return _capabilities; }

    // Add a capability
    void addCapability(const String& capability) {
        _capabilities.push_back(capability);
    }

private:
    // Device name, should be loaded from settings
    static String _deviceName;

    // Firmware version from generated version.h file
    static String _firmwareVersion;
    static String _buildDate;
    static String _buildTime;
    static String _buildTimestamp;
    static String _versionBranch;
    static String _versionHash;
    static String _versionTag;
    // Will include info about the hardware revision
    // Eg: srd001-sd1-tp1-dp1-ld1 means SRDriver revision 001, sd card rev 1, temp rev 1, display rev1, led rev1
    //     srd001-sd1-tp1-dp1-ld2 would indicate a later display revision
    static String _deviceVersion;

    static std::vector<String> _capabilities;
};
