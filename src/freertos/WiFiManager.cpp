#include "WiFiManager.h"
#include "hal/ble/BLEManager.h"

void WiFiManager::updateBLEStatus() {
    if (_bleManager) {
        // Update WiFi status
        _bleManager->setWiFiStatus(getStatus());
        
        // Update IP address
        String ip = getIPAddress();
        if (ip.length() > 0) {
            _bleManager->setIPAddress(ip);
        }
        
        // Log the status update
        LOG_DEBUGF("WiFiManager: Updated BLE status: %s, IP: %s", 
                  getStatus().c_str(), ip.c_str());
    }
}
