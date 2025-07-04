#include "BLEManager.h"
#include "GlobalState.h"
#include "utility/strings.hpp"

BLEManager::BLEManager(DeviceState& state)
    : deviceState(state) {}

void BLEManager::begin() {
    // TODO: Move BLE.begin(), set device name, add characteristics, etc. here
    setupCharacteristics();
    BLE.advertise();
}

void BLEManager::poll() {
    // TODO: Call this in loop() to handle BLE events
    handleEvents();
}

void BLEManager::setOnSettingChanged(OnSettingChangedCallback cb) {
    onSettingChanged = cb;
}

void BLEManager::setupCharacteristics() {
    // TODO: Add all characteristics and descriptors to services
}

void BLEManager::handleEvents() {
    // TODO: Handle BLE events, check for characteristic writes, update deviceState, call onSettingChanged if needed
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
