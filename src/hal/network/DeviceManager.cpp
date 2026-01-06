#include "DeviceManager.h"
#include <ArduinoJson.h>

DeviceManager& DeviceManager::getInstance() {
    static DeviceManager instance;
    return instance;
}

DeviceManager::~DeviceManager() {
    disconnectAll();
}

bool DeviceManager::connectDevice(const String& ipAddress, const String& name) {
    // Check if device already exists
    for (auto& device : _devices) {
        if (device.ipAddress == ipAddress) {
            LOG_WARNF_COMPONENT("DeviceManager", "Device %s already exists", ipAddress.c_str());
            return false;
        }
    }
    
    // Check max devices limit
    if (_devices.size() >= MAX_DEVICES_LIMIT) {
        LOG_ERRORF_COMPONENT("DeviceManager", "Maximum devices (%d) reached", MAX_DEVICES_LIMIT);
        return false;
    }
    
    LOG_INFOF_COMPONENT("DeviceManager", "Adding device: %s", ipAddress.c_str());
    
    // Create device info
    DeviceInfo deviceInfo;
    deviceInfo.ipAddress = ipAddress;
    deviceInfo.displayName = name.length() > 0 ? name : generateDisplayName();
    deviceInfo.client = new SRWebSocketClient(ipAddress);
    deviceInfo.autoReconnect = false;
    deviceInfo.lastActivity = 0;
    
    if (!deviceInfo.client) {
        LOG_ERROR_COMPONENT("DeviceManager", "Failed to allocate SRWebSocketClient");
        return false;
    }

    deviceInfo.client->setAutoReconnect(false);
    
    // Attempt connection
    if (deviceInfo.client->connect()) {
        _devices.push_back(deviceInfo);
        LOG_INFOF_COMPONENT("DeviceManager", "Device %s (%s) added successfully", 
                           ipAddress.c_str(), deviceInfo.displayName.c_str());
        return true;
    } else {
        LOG_ERRORF_COMPONENT("DeviceManager", "Failed to connect to %s", ipAddress.c_str());
        delete deviceInfo.client;
        return false;
    }
}

void DeviceManager::disconnectDevice(const String& ipAddress) {
    for (auto it = _devices.begin(); it != _devices.end(); ++it) {
        if (it->ipAddress == ipAddress) {
            LOG_INFOF_COMPONENT("DeviceManager", "Removing device: %s", ipAddress.c_str());
            
            if (it->client) {
                it->client->disconnect();
                delete it->client;
            }
            
            _devices.erase(it);
            return;
        }
    }
    
    LOG_WARNF_COMPONENT("DeviceManager", "Device %s not found", ipAddress.c_str());
}

void DeviceManager::disconnectAll() {
    LOG_INFO_COMPONENT("DeviceManager", "Disconnecting all devices");
    
    for (auto& device : _devices) {
        if (device.client) {
            device.client->disconnect();
            delete device.client;
            device.client = nullptr;
        }
    }
    
    _devices.clear();
}

bool DeviceManager::sendCommandToDevice(const String& ipAddress, const String& jsonCommand) {
    DeviceInfo* device = getDevice(ipAddress);
    if (!device || !device->client) {
        LOG_WARNF_COMPONENT("DeviceManager", "Device %s not found", ipAddress.c_str());
        return false;
    }
    
    return device->client->sendCommand(jsonCommand);
}

bool DeviceManager::sendBrightnessToDevice(const String& ipAddress, uint8_t brightness) {
    DeviceInfo* device = getDevice(ipAddress);
    if (!device || !device->client) {
        LOG_WARNF_COMPONENT("DeviceManager", "Device %s not found", ipAddress.c_str());
        return false;
    }
    
    return device->client->sendBrightness(brightness);
}

bool DeviceManager::broadcastCommand(const String& jsonCommand) {
    bool allSuccess = true;
    size_t sentCount = 0;
    
    for (auto& device : _devices) {
        if (device.client && device.client->isConnected()) {
            if (device.client->sendCommand(jsonCommand)) {
                sentCount++;
            } else {
                allSuccess = false;
            }
        }
    }
    
    LOG_DEBUGF_COMPONENT("DeviceManager", "Broadcast command sent to %d devices", sentCount);
    return allSuccess;
}

DeviceManager::DeviceInfo* DeviceManager::getDevice(const String& ipAddress) {
    for (auto& device : _devices) {
        if (device.ipAddress == ipAddress) {
            return &device;
        }
    }
    return nullptr;
}

size_t DeviceManager::getConnectedCount() const {
    size_t count = 0;
    for (const auto& device : _devices) {
        if (device.client && device.client->isConnected()) {
            count++;
        }
    }
    return count;
}

bool DeviceManager::isDeviceConnected(const String& ipAddress) const {
    for (const auto& device : _devices) {
        if (device.ipAddress == ipAddress) {
            return device.client && device.client->isConnected();
        }
    }
    return false;
}

String DeviceManager::getDeviceListJSON() const {
    StaticJsonDocument<2048> doc;
    JsonArray devices = doc.createNestedArray("devices");
    
    for (const auto& device : _devices) {
        JsonObject deviceObj = devices.createNestedObject();
        deviceObj["ip"] = device.ipAddress;
        deviceObj["name"] = device.displayName;
        deviceObj["connected"] = device.client ? device.client->isConnected() : false;
        deviceObj["lastActivity"] = device.lastActivity;
        
        if (device.client) {
            deviceObj["state"] = static_cast<int>(device.client->getState());
            deviceObj["lastStatus"] = device.client->getLastStatus();
        }
    }
    
    doc["total"] = _devices.size();
    doc["connected"] = getConnectedCount();
    
    String result;
    serializeJson(doc, result);
    return result;
}

String DeviceManager::getDeviceStatus(const String& ipAddress) const {
    DeviceInfo* device = const_cast<DeviceManager*>(this)->getDevice(ipAddress);
    if (!device || !device->client) {
        return "";
    }
    
    return device->client->getLastStatus();
}

void DeviceManager::update() {
    // Update all clients
    for (auto& device : _devices) {
        if (device.client) {
            device.client->update();
            
            // Update last activity if connected
            if (device.client->isConnected()) {
                if (!device.autoReconnect) {
                    device.autoReconnect = true;
                    device.client->setAutoReconnect(true);
                    LOG_INFOF_COMPONENT("DeviceManager", "Auto-reconnect enabled for device: %s", device.ipAddress.c_str());
                }
                device.lastActivity = device.client->getLastActivity();
            }
        }
    }
    
    // Check and reconnect devices (clean auto-reconnect logic)
    checkAndReconnectDevices();
}

String DeviceManager::generateDisplayName() const {
    // Generate "SRDriver 1", "SRDriver 2", etc.
    int deviceNumber = _devices.size() + 1;
    return "SRDriver " + String(deviceNumber);
}

void DeviceManager::checkAndReconnectDevices() {
    // Clean auto-reconnect logic - check each device
    for (auto& device : _devices) {
        if (!device.client) continue;
        
        // If device should auto-reconnect and is not connected, let client handle it
        if (device.autoReconnect && !device.client->isConnected()) {
            // The client's update() method will handle reconnection
            // We just need to make sure update() is being called (which it is)
        }
    }
}

