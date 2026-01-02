#include "WiFiManager.h"
#include "hal/ble/BLEManager.h"
#include "lights/LEDManager.h"

void WiFiManager::updateBLEStatus()
{
    if (_bleManager)
    {
        // Update WiFi status
        _bleManager->setWiFiStatus(getStatus());

        // Update IP address
        String ip = getIPAddress();
        if (ip.length() > 0)
        {
            _bleManager->setIPAddress(ip);
        }

        // Log the status update
        // LOG_DEBUGF_COMPONENT("WiFiManager", "Updated BLE status: %s, IP: %s", 
        //           getStatus().c_str(), ip.c_str());
        // LOG_DEBUG_COMPONENT("WiFiManager", String("Updated BLE status: " + getStatus() + ", IP: " + ip).c_str());
    }
}

void WiFiManager::startWebSocketServer()
{
    LOG_DEBUG_COMPONENT("WiFiManager", "startWebSocketServer() called");

    if (_webSocketServer)
    {
        LOG_WARN_COMPONENT("WiFiManager", "WebSocket server already started");
        return; // Already started
    }

    if (!_ledManager)
    {
        LOG_ERROR_COMPONENT("WiFiManager", "Cannot start WebSocket server - LEDManager not set");
        return;
    }

    LOG_DEBUG_COMPONENT("WiFiManager", "Creating SRWebSocketServer instance...");
    try
    {
        _webSocketServer = new SRWebSocketServer(_ledManager, 8080);
        // LOG_DEBUG_COMPONENT("WiFiManager", "SRWebSocketServer instance created");
        LOG_DEBUG_COMPONENT("WiFiManager", "SRWebSocketServer instance created");

        LOG_DEBUG_COMPONENT("WiFiManager", "Starting WebSocket server...");
        _webSocketServer->start();
        LOG_INFO_COMPONENT("WiFiManager", "WebSocket server started successfully");
    }
    catch (const std::exception &e)
    {
        LOG_ERRORF_COMPONENT("WiFiManager", "Exception in startWebSocketServer: %s", e.what());
        if (_webSocketServer)
        {
            delete _webSocketServer;
            _webSocketServer = nullptr;
        }
        throw;
    }
    catch (...)
    {
        LOG_ERROR_COMPONENT("WiFiManager", "Unknown exception in startWebSocketServer");
        if (_webSocketServer)
        {
            delete _webSocketServer;
            _webSocketServer = nullptr;
        }
        throw;
    }
}

void WiFiManager::stopWebSocketServer()
{
    if (!_webSocketServer) return;

    _webSocketServer->stop();
    delete _webSocketServer;
    _webSocketServer = nullptr;
    LOG_INFO_COMPONENT("WiFiManager", "WebSocket server stopped");
}

bool WiFiManager::isWebSocketServerRunning() const
{
    return _webSocketServer && _webSocketServer->isRunning();
}

void WiFiManager::broadcastToClients(const String &message)
{
    if (_webSocketServer)
    {
        _webSocketServer->broadcastMessage(message);
    }
}

void WiFiManager::run()
{
    LOG_INFO_COMPONENT("WiFiManager", "WiFiManager task started");
    LOG_INFOF_COMPONENT("WiFiManager", "Update interval: %d ms", _updateIntervalMs);

    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true)
    {

        // Handle WiFi connection if needed (only if we have credentials)
        if (_shouldConnect && !isConnected() && _ssid.length() > 0)
        {
            attemptConnection();
        }

        // Update BLE status if connected
        if (isConnected() && _bleManager)
        {
            updateBLEStatus();

            // Start WebSocket server if not already started
            if (!_webSocketServer)
            {
                LOG_INFO_COMPONENT("WiFiManager", "WiFi connected, attempting to start WebSocket server...");
                try
                {
                    startWebSocketServer();
                    LOG_INFO_COMPONENT("WiFiManager", "WebSocket server started successfully");
                }
                catch (const std::exception &e)
                {
                    LOG_ERRORF_COMPONENT("WiFiManager", "WebSocket server failed to start: %s", e.what());
                }
                catch (...)
                {
                    LOG_ERROR_COMPONENT("WiFiManager", "WebSocket server failed to start (unknown error)");
                }
            }
        }
        else
        {
            // LOG_DEBUGF_COMPONENT("WiFiManager", "Not updating BLE status - isConnected: %s, _bleManager: %p", 
            //           isConnected() ? "true" : "false", _bleManager);
        }

        // Handle WebSocket server if connected
        if (isConnected() && _webSocketServer)
        {
            _webSocketServer->update(); // Just tick it!
        }

        // Increment update counter
        _updateCount++;

        // Log WiFi status every 10 seconds
        uint32_t now = millis();
        if (now - _lastStatusLog > 10000)
        {
            // LOG_DEBUGF_COMPONENT("WiFiManager", "WiFi Update - Cycles: %d, Status: %s, IP: %s", 
            //           _updateCount, getStatus().c_str(), getIPAddress().c_str());
            _updateCount = 0;
            _lastStatusLog = now;
        }

        // Sleep until next update
        SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
    }
}

void WiFiManager::attemptConnection()
{
    static auto lastAttemptTime = millis();
    // Don't throttle the first connection attempt
    static bool hasTriedToConnect = false;
    // Only attempt connection every 5 seconds to give previous attempt time to complete
    if (hasTriedToConnect && millis() - lastAttemptTime < 2000)
    {
        return;
    }
    lastAttemptTime = millis();
    hasTriedToConnect = true;

    if (_connectionAttempts >= _maxConnectionAttempts)
    {
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
    if (currentStatus != WL_DISCONNECTED && currentStatus != WL_IDLE_STATUS && currentStatus != 255)
    {
        LOG_DEBUG_COMPONENT("WiFiManager", "Disconnecting WiFi before new connection attempt");
        WiFi.disconnect(false, false);  // false = don't sleep, false = don't erase credentials
    }

    // Scan networks and report strength for each network
    int8_t rssi = 0;
    int32_t scanResult = WiFi.scanNetworks();
    std::vector<String> scannedNetworks;
    for (int i = 0; i < scanResult; ++i) {
        scannedNetworks.push_back(WiFi.SSID(i));
    }
    // if (scanResult == WIFI_SCAN_RUNNING)
    // {
    //     LOG_DEBUG_COMPONENT("WiFiManager", "WiFi scan running, waiting for result...");
    // }
    // if (scanResult > 0)
    // {
    //     LOG_DEBUGF_COMPONENT("WiFiManager", "Found %d networks", scanResult);
    //     for (int i = 0; i < scanResult; ++i)
    //     {
    //         rssi = WiFi.RSSI(i);

    //         LOG_DEBUGF_COMPONENT("WiFiManager", "Network %d: %s, RSSI: %d dBm", i, WiFi.SSID(i).c_str(), rssi);
    //     }
    // }
    // else
    // {
    //     LOG_DEBUG_COMPONENT("WiFiManager", "No networks found");
    // }

    // LOG_DEBUGF_COMPONENT("WiFiManager", "Starting connection attempt %d/%d to '%s'",
    //     _connectionAttempts, _maxConnectionAttempts, _ssid.c_str());


    const auto find_in_known_networks = [this](const String &ssid) {
        return std::find_if(_knownNetworks.begin(), _knownNetworks.end(), [ssid](const NetworkCredentials &network) { return network.ssid == ssid; });
    };

    const auto find_in_scanned_networks = [this, &scannedNetworks](const String &ssid) {
        return std::find_if(scannedNetworks.begin(), scannedNetworks.end(), [ssid](const String &network) { return network == ssid; });
    };

    // First, see if the primary network was discovered, if so then we'll connect to it
    String networkToConnectToSSID = _ssid;
    String networkToConnectToPassword = _password;
    
    const auto primaryNetworkIt = find_in_scanned_networks(_ssid);
    if (primaryNetworkIt != scannedNetworks.end()) {
        LOG_DEBUGF_COMPONENT("WiFiManager", "Found primary network in scanned networks: %s", _ssid.c_str());
        networkToConnectToSSID = *primaryNetworkIt;
        networkToConnectToPassword = _password;
    } else {
        LOG_DEBUGF_COMPONENT("WiFiManager", "Primary network not found in scanned networks: %s", _ssid.c_str());
        // We'll see if we recognize any of the scanned networks
        bool foundKnownNetwork = false;
        for (const auto &scanned : scannedNetworks) {
            const auto knownNetworkIt = find_in_known_networks(scanned);
            if (knownNetworkIt != _knownNetworks.end()) {
                LOG_DEBUGF_COMPONENT("WiFiManager", "Found known network in scanned networks: %s", scanned.c_str());
                foundKnownNetwork = true;
                networkToConnectToSSID = knownNetworkIt->ssid;
                networkToConnectToPassword = knownNetworkIt->password;
                LOG_DEBUGF_COMPONENT("WiFiManager", "Using credentials for known network: %s %s", networkToConnectToSSID.c_str(), networkToConnectToPassword.c_str());
                break;
            }
        }

        if (!foundKnownNetwork) {
            LOG_DEBUGF_COMPONENT("WiFiManager", "No known networks found in scanned networks");
            networkToConnectToSSID = _ssid;
            networkToConnectToPassword = _password;
            LOG_DEBUGF_COMPONENT("WiFiManager", "Using credentials for primary network: %s %s", networkToConnectToSSID.c_str(), networkToConnectToPassword.c_str());
        }
    }


    // Start connection
    WiFi.begin(networkToConnectToSSID.c_str(), networkToConnectToPassword.c_str());

    uint8_t waitResult = WiFi.waitForConnectResult(5000);
    wl_status_t status = static_cast<wl_status_t>(waitResult);

    // Check if connection succeeded
    LOG_DEBUGF_COMPONENT("WiFiManager", "Connection result: %d (0=idle, 1=no_ssid, 3=connected, 4=failed, 5=lost, 6=disconnected)", status);

    // Double-check status after a brief delay (sometimes waitForConnectResult returns early)
    // Status 5 (WL_CONNECTION_LOST) means it connected but then lost it - verify if it's actually connected
    if (status == WL_CONNECTED || status == WL_CONNECTION_LOST)
    {
        delay(500);
        wl_status_t verifyStatus = WiFi.status();
        LOG_DEBUGF_COMPONENT("WiFiManager", "Verification status after delay: %d", verifyStatus);

        if (verifyStatus == WL_CONNECTED)
        {
            // Actually connected! Update status
            status = WL_CONNECTED;
        }
    }

    if (status == WL_CONNECTED)
    {
        String ip = getIPAddress();
        LOG_INFOF_COMPONENT("WiFiManager", "✅ Connected to '%s' with IP: %s", _ssid.c_str(), ip.c_str());
        _shouldConnect = false;
        _connectionAttempts = 0;

        // Start WebSocket server on successful connection
        LOG_INFO_COMPONENT("WiFiManager", "Attempting to start WebSocket server...");
        try
        {
            startWebSocketServer();
            LOG_INFO_COMPONENT("WiFiManager", "WebSocket server started successfully");
        }
        catch (const std::exception &e)
        {
            LOG_ERRORF_COMPONENT("WiFiManager", "WebSocket server failed to start: %s", e.what());
        }
        catch (...)
        {
            LOG_ERROR_COMPONENT("WiFiManager", "WebSocket server failed to start (unknown error)");
        }

        // Update BLE characteristics immediately
        updateBLEStatus();
    }
    else
    {
        // Connection failed - log the reason
        const char *reason = "unknown";
        switch (status)
        {
            case WL_NO_SSID_AVAIL: reason = "SSID not found"; break;
            case WL_CONNECT_FAILED: reason = "Connection failed (wrong password?)"; break;
            case WL_CONNECTION_LOST: reason = "Connection lost"; break;
            case WL_DISCONNECTED: reason = "Disconnected"; break;
            case WL_IDLE_STATUS: reason = "WiFi idle/not initialized"; break;
        }
        LOG_WARNF_COMPONENT("WiFiManager", "Connection attempt %d/%d failed: %s (status %d)",
            _connectionAttempts, _maxConnectionAttempts, reason, status);

        // Check if we've exceeded max attempts
        if (_connectionAttempts >= _maxConnectionAttempts)
        {
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
