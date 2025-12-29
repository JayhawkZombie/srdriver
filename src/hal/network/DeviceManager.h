#pragma once

#include "WebSocketClient.h"
#include "freertos/LogManager.h"
#include <vector>
#include <ArduinoJson.h>

/**
 * DeviceManager - Singleton for managing multiple WebSocket device connections
 * 
 * Features:
 * - Manage list of connected devices (max 10, configurable)
 * - Add/remove devices by IP address
 * - Route commands to specific devices
 * - Track connection status for all devices
 * - Auto-reconnect management for all devices
 * - Device identification and naming
 */
class DeviceManager {
public:
    struct DeviceInfo {
        String ipAddress;
        String displayName;  // "SRDriver 1", "SRDriver 2", etc.
        SRWebSocketClient* client;
        unsigned long lastActivity;
        bool autoReconnect;
        
        DeviceInfo() : client(nullptr), lastActivity(0), autoReconnect(true) {}
    };
    
    static DeviceManager& getInstance();
    
    // Device management
    bool connectDevice(const String& ipAddress, const String& name = "");
    void disconnectDevice(const String& ipAddress);
    void disconnectAll();
    
    // Command sending
    bool sendCommandToDevice(const String& ipAddress, const String& jsonCommand);
    bool sendBrightnessToDevice(const String& ipAddress, uint8_t brightness);
    bool broadcastCommand(const String& jsonCommand);  // Send to all connected devices
    
    // Device access
    DeviceInfo* getDevice(const String& ipAddress);
    size_t getDeviceCount() const { return _devices.size(); }
    size_t getConnectedCount() const;
    bool isDeviceConnected(const String& ipAddress) const;
    
    // Status
    String getDeviceListJSON() const;  // For UI display
    String getDeviceStatus(const String& ipAddress) const;
    
    // Must call in loop()
    void update();
    
private:
    DeviceManager() = default;
    ~DeviceManager();
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    
    std::vector<DeviceInfo> _devices;
    static constexpr size_t MAX_DEVICES_LIMIT = MAX_DEVICES;  // From WebSocketClient.h
    
    String generateDisplayName() const;  // Auto-generate "SRDriver N"
    void checkAndReconnectDevices();  // Clean auto-reconnect logic
};

