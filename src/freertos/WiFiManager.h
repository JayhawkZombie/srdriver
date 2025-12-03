#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include <WiFi.h>
#include "hal/network/WebSocketServer.h"

// Forward declaration
class BLEManager;

/**
 * WiFiManager - FreeRTOS task for WiFi management
 * 
 * Handles:
 * - WiFi connection management
 * - WiFi status updates
 * - IP address management
 * - BLE integration for WiFi status
 */
class WiFiManager : public SRTask {
public:
    WiFiManager(uint32_t updateIntervalMs = 10,  // 10ms for WiFi
                uint32_t stackSize = 8192,
                UBaseType_t priority = tskIDLE_PRIORITY + 1,
                BaseType_t core = 0)  // Pin to core 0 (WiFi/BLE core)
        : SRTask("WiFiManager", stackSize, priority, core),
          _updateIntervalMs(updateIntervalMs),
          _updateCount(0),
          _lastStatusLog(0),
          _bleManager(nullptr),
          _connectionAttempts(0),
          _maxConnectionAttempts(10),
          _connectionTimeoutMs(10000) {}
    
    /**
     * Set BLE manager for status updates
     */
    void setBLEManager(BLEManager* manager) {
        _bleManager = manager;
    }
    
    /**
     * Set LED manager for WebSocket command routing
     */
    void setLEDManager(LEDManager* ledManager) {
        _ledManager = ledManager;
    }
    
    /**
     * Check for saved credentials and attempt connection
     */
    void checkSavedCredentials() {
        // This will be called after preferences are loaded
        LOG_DEBUGF_COMPONENT("WiFiManager", "checkSavedCredentials() called - SSID length: %d, Password length: %d", _ssid.length(), _password.length());
        if (_ssid.length() > 0 && _password.length() > 0) {
            LOG_DEBUGF_COMPONENT("WiFiManager", "Found saved credentials for '%s', attempting connection", _ssid.c_str());
            _shouldConnect = true;
            _connectionAttempts = 0;
        } else {
            LOG_DEBUG_COMPONENT("WiFiManager", "No saved credentials found - skipping auto-connect");
        }
    }
    
    /**
     * Set WiFi credentials and trigger connection
     */
    void setCredentials(const String& ssid, const String& password) {
        _ssid = ssid;
        _password = password;
        _shouldConnect = true;
        _connectionAttempts = 0; // Reset attempts for new credentials
        LOG_DEBUGF_COMPONENT("WiFiManager", "Credentials set for '%s', will attempt connection", ssid.c_str());
    }
    
    /**
     * Get current connection status
     */
    bool isConnected() const {
        wl_status_t status = WiFi.status();
        return status == WL_CONNECTED;
    }
    
    /**
     * Get current IP address
     */
    String getIPAddress() const {
        if (isConnected()) {
            IPAddress ip = WiFi.localIP();
            if (ip != INADDR_NONE) {
                return ip.toString();
            }
        }
        return "";
    }
    
    /**
     * Get current status string
     */
    String getStatus() const {
        switch (WiFi.status()) {
            case WL_CONNECTED:
                return "connected";
            case WL_NO_SSID_AVAIL:
                return "no_ssid";
            case WL_CONNECT_FAILED:
                return "connect_failed";
            case WL_CONNECTION_LOST:
                return "connection_lost";
            case WL_DISCONNECTED:
                return "disconnected";
            default:
                return "unknown";
        }
    }
    
    /**
     * Get update count
     */
    uint32_t getUpdateCount() const { return _updateCount; }
    
    /**
     * Set update interval
     */
    void setUpdateInterval(uint32_t intervalMs) {
        _updateIntervalMs = intervalMs;
    }
    
    /**
     * WebSocket server management
     */
    void startWebSocketServer();
    void stopWebSocketServer();
    bool isWebSocketServerRunning() const;
    void broadcastToClients(const String& message);

protected:
    /**
     * Main task loop - handles WiFi connection and status updates
     */
    void run() override {
        LOG_INFO_COMPONENT("WiFiManager", "WiFiManager task started");
        LOG_INFOF_COMPONENT("WiFiManager", "Update interval: %d ms", _updateIntervalMs);
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            
            // Handle WiFi connection if needed (only if we have credentials)
            if (_shouldConnect && !isConnected() && _ssid.length() > 0) {
                attemptConnection();
            }
        
        // Update BLE status if connected
        if (isConnected() && _bleManager) {
            updateBLEStatus();
            
            // Start WebSocket server if not already started
            if (!_webSocketServer) {
                LOG_INFO_COMPONENT("WiFiManager", "WiFi connected, attempting to start WebSocket server...");
                try {
                    startWebSocketServer();
                    LOG_INFO_COMPONENT("WiFiManager", "WebSocket server started successfully");
                } catch (const std::exception& e) {
                    LOG_ERRORF_COMPONENT("WiFiManager", "WebSocket server failed to start: %s", e.what());
                } catch (...) {
                    LOG_ERROR_COMPONENT("WiFiManager", "WebSocket server failed to start (unknown error)");
                }
            }
        } else {
            // LOG_DEBUGF_COMPONENT("WiFiManager", "Not updating BLE status - isConnected: %s, _bleManager: %p", 
            //           isConnected() ? "true" : "false", _bleManager);
        }
        
        // Handle WebSocket server if connected
        if (isConnected() && _webSocketServer) {
            _webSocketServer->update(); // Just tick it!
        }
            
            // Increment update counter
            _updateCount++;
            
            // Log WiFi status every 10 seconds
            uint32_t now = millis();
            if (now - _lastStatusLog > 10000) {
                // LOG_DEBUGF_COMPONENT("WiFiManager", "WiFi Update - Cycles: %d, Status: %s, IP: %s", 
                //           _updateCount, getStatus().c_str(), getIPAddress().c_str());
                _updateCount = 0;
                _lastStatusLog = now;
            }
            
            // Sleep until next update
            SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
        }
    }

private:
    BLEManager* _bleManager;
    LEDManager* _ledManager;
    SRWebSocketServer* _webSocketServer = nullptr;
    uint32_t _updateIntervalMs;
    uint32_t _updateCount;
    uint32_t _lastStatusLog;
    
    // WiFi credentials
    String _ssid;
    String _password;
    bool _shouldConnect;
    
    // Connection management
    uint32_t _connectionAttempts;
    uint32_t _maxConnectionAttempts;
    uint32_t _connectionTimeoutMs;
    uint32_t _connectionStartTime;
    
    /**
     * Attempt WiFi connection
     */
    void attemptConnection() {
        static auto lastAttemptTime = millis();
        // Don't throttle the first connection attempt
        static bool hasTriedToConnect = false;
        // Only attempt connection every 5 seconds to give previous attempt time to complete
        if (hasTriedToConnect && millis() - lastAttemptTime < 2000) {
            return;
        }
        lastAttemptTime = millis();
        hasTriedToConnect = true;
        
        if (_connectionAttempts >= _maxConnectionAttempts) {
            LOG_ERROR_COMPONENT("WiFiManager", "Max connection attempts reached, giving up");
            _shouldConnect = false;
            return;
        }

        ++_connectionAttempts;
        _connectionStartTime = millis();
        
        // Check current WiFi status
        wl_status_t currentStatus = WiFi.status();
        LOG_DEBUGF_COMPONENT("WiFiManager", "Current WiFi status before attempt: %d", currentStatus);
        
        // Disconnect any existing connection/attempt before starting new one
        // Don't put WiFi to sleep (false), don't erase credentials (false) - we want to keep them
        if (currentStatus != WL_DISCONNECTED && currentStatus != WL_IDLE_STATUS && currentStatus != 255) {
            LOG_DEBUG_COMPONENT("WiFiManager", "Disconnecting WiFi before new connection attempt");
            WiFi.disconnect(false, false);  // false = don't sleep, false = don't erase credentials
            // delay(1000);  // Give it more time to fully disconnect
        }

        // Scan networks and report strength for each network
        int8_t rssi = 0;
        int32_t scanResult = WiFi.scanNetworks();
        if (scanResult == WIFI_SCAN_RUNNING) {
            LOG_DEBUG_COMPONENT("WiFiManager", "WiFi scan running, waiting for result...");
        }
        if (scanResult > 0) {
            LOG_DEBUGF_COMPONENT("WiFiManager", "Found %d networks", scanResult);
            for (int i = 0; i < scanResult; ++i) {
                rssi = WiFi.RSSI(i);
                
                LOG_DEBUGF_COMPONENT("WiFiManager", "Network %d: %s, RSSI: %d dBm", i, WiFi.SSID(i).c_str(), rssi);
            }
        } else {
            LOG_DEBUG_COMPONENT("WiFiManager", "No networks found");
        }
        
        LOG_DEBUGF_COMPONENT("WiFiManager", "Starting connection attempt %d/%d to '%s'", 
            _connectionAttempts, _maxConnectionAttempts, _ssid.c_str());
        
        // Start connection
        WiFi.begin(_ssid.c_str(), _password.c_str());
        
        uint8_t waitResult = WiFi.waitForConnectResult(5000);
        wl_status_t status = static_cast<wl_status_t>(waitResult);
        
        // Check if connection succeeded
        LOG_DEBUGF_COMPONENT("WiFiManager", "Connection result: %d (0=idle, 1=no_ssid, 3=connected, 4=failed, 5=lost, 6=disconnected)", status);
        
        // Double-check status after a brief delay (sometimes waitForConnectResult returns early)
        // Status 5 (WL_CONNECTION_LOST) means it connected but then lost it - verify if it's actually connected
        if (status == WL_CONNECTED || status == WL_CONNECTION_LOST) {
            delay(500);
            wl_status_t verifyStatus = WiFi.status();
            LOG_DEBUGF_COMPONENT("WiFiManager", "Verification status after delay: %d", verifyStatus);
            
            if (verifyStatus == WL_CONNECTED) {
                // Actually connected! Update status
                status = WL_CONNECTED;
                LOG_DEBUG_COMPONENT("WiFiManager", "Connection verified - actually connected");
            } else if (status == WL_CONNECTION_LOST && verifyStatus == WL_CONNECTION_LOST) {
                // Connection was lost - this is a real failure
                LOG_WARNF_COMPONENT("WiFiManager", "Connection established but immediately lost (status 5) - possible BLE interference");
            }
        }
        
        if (status == WL_CONNECTED) {
            String ip = getIPAddress();
            LOG_INFOF_COMPONENT("WiFiManager", "✅ Connected to '%s' with IP: %s", 
                     _ssid.c_str(), ip.c_str());
            _shouldConnect = false;
            _connectionAttempts = 0;
            
            // Start WebSocket server on successful connection
            LOG_INFO_COMPONENT("WiFiManager", "Attempting to start WebSocket server...");
            try {
                startWebSocketServer();
                LOG_INFO_COMPONENT("WiFiManager", "WebSocket server started successfully");
            } catch (const std::exception& e) {
                LOG_ERRORF_COMPONENT("WiFiManager", "WebSocket server failed to start: %s", e.what());
            } catch (...) {
                LOG_ERROR_COMPONENT("WiFiManager", "WebSocket server failed to start (unknown error)");
            }
            
            // Update BLE characteristics immediately
            updateBLEStatus();
        } else {
            // Connection failed - log the reason
            const char* reason = "unknown";
            switch (status) {
                case WL_NO_SSID_AVAIL: reason = "SSID not found"; break;
                case WL_CONNECT_FAILED: reason = "Connection failed (wrong password?)"; break;
                case WL_CONNECTION_LOST: reason = "Connection lost"; break;
                case WL_DISCONNECTED: reason = "Disconnected"; break;
                case WL_IDLE_STATUS: reason = "WiFi idle/not initialized"; break;
            }
            LOG_WARNF_COMPONENT("WiFiManager", "Connection attempt %d/%d failed: %s (status %d)", 
                _connectionAttempts, _maxConnectionAttempts, reason, status);
            
            // Check if we've exceeded max attempts
            if (_connectionAttempts >= _maxConnectionAttempts) {
                LOG_ERROR_COMPONENT("WiFiManager", "Max connection attempts reached, giving up");
                _shouldConnect = false;
                _connectionAttempts = 0;
                
                // Stop WebSocket server on connection failure
                stopWebSocketServer();
                
                // Update BLE status to failed
                updateBLEStatus();
            }
        }
    }
    
    /**
     * Update BLE characteristics with WiFi status
     */
    void updateBLEStatus();
};
