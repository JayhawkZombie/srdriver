#include "WiFiManager.h"
#include "hal/ble/BLEManager.h"
#include "lights/LEDManager.h"

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
        // LOG_DEBUGF("WiFiManager: Updated BLE status: %s, IP: %s", 
        //           getStatus().c_str(), ip.c_str());
        // LOG_DEBUG_COMPONENT("WiFiManager", String("Updated BLE status: " + getStatus() + ", IP: " + ip).c_str());
    }
}

void WiFiManager::startWebSocketServer() {
    LOG_DEBUG("WiFiManager::startWebSocketServer() called");
    
    if (_webSocketServer) {
        LOG_WARN("WebSocket server already started");
        return; // Already started
    }
    
    if (!_ledManager) {
        LOG_ERROR("Cannot start WebSocket server - LEDManager not set");
        return;
    }
    
    LOG_DEBUG("WiFiManager: Creating SRWebSocketServer instance...");
    try {
        _webSocketServer = new SRWebSocketServer(_ledManager, 8080);
        // LOG_DEBUG("WiFiManager: SRWebSocketServer instance created");
        LOG_DEBUG_COMPONENT("WiFiManager", "SRWebSocketServer instance created");
        
        LOG_DEBUG("WiFiManager: Starting WebSocket server...");
        _webSocketServer->start();
        LOG_INFO("WiFiManager: WebSocket server started successfully");
    } catch (const std::exception& e) {
        LOG_ERRORF("WiFiManager: Exception in startWebSocketServer: %s", e.what());
        if (_webSocketServer) {
            delete _webSocketServer;
            _webSocketServer = nullptr;
        }
        throw;
    } catch (...) {
        LOG_ERROR("WiFiManager: Unknown exception in startWebSocketServer");
        if (_webSocketServer) {
            delete _webSocketServer;
            _webSocketServer = nullptr;
        }
        throw;
    }
}

void WiFiManager::stopWebSocketServer() {
    if (!_webSocketServer) return;
    
    _webSocketServer->stop();
    delete _webSocketServer;
    _webSocketServer = nullptr;
    LOG_INFO("WebSocket server stopped");
}

bool WiFiManager::isWebSocketServerRunning() const {
    return _webSocketServer && _webSocketServer->isRunning();
}

void WiFiManager::broadcastToClients(const String& message) {
    if (_webSocketServer) {
        _webSocketServer->broadcastMessage(message);
    }
}
