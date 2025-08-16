#pragma once

#include <ArduinoBLE.h>
#include <functional>
#include <vector>
#include <String.h>

struct BLE2904_Data {
    uint8_t m_format;
    int8_t m_exponent;
    uint16_t m_unit;
    uint8_t m_namespace;
    uint16_t m_description;
} __attribute__((packed));

// Characteristic registry structure
struct BLECharacteristicInfo {
    // UUIDs - component provides everything
    String characteristicUuid;
    String descriptorUuid;        // Usually "2901" for name
    String formatDescriptorUuid;  // Usually "2904" for format
    
    // Metadata
    String name;
    String description;
    
    // Characteristic properties
    bool isWritable;
    bool isReadable;
    bool isNotifiable;
    int maxValueLength;
    
    // BLE objects (registry will create these)
    BLECharacteristic* characteristic;
    BLEDescriptor* descriptor;
    BLEDescriptor* formatDescriptor;
    
    // Callbacks
    std::function<void(const unsigned char* value, size_t length)> onWrite;
    std::function<String()> onRead;
    std::function<void()> onSubscribe;  // Optional: when client subscribes
    std::function<void()> onUnsubscribe; // Optional: when client unsubscribes
    
    // Format data for 2904 descriptor
    BLE2904_Data formatData;
};

class BLECharacteristicRegistry {
private:
    std::vector<BLECharacteristicInfo> characteristics;
    BLEService* service;
    
public:
    BLECharacteristicRegistry(BLEService* svc) : service(svc) {}
    void registerCharacteristic(BLECharacteristicInfo& info);
    void unregisterCharacteristic(const String& uuid);
    void updateAllCharacteristics();
    void handleCharacteristicWrite(const String& uuid, const unsigned char* value, size_t length);
    const std::vector<BLECharacteristicInfo>& getCharacteristics() const { return characteristics; }
    
private:
    void createBLEObjects(BLECharacteristicInfo& info);
    void addToService(BLECharacteristicInfo& info);
}; 