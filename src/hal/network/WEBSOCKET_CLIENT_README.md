# WebSocket Client System for CrowPanel

## Overview

This system enables the CrowPanel to act as a controller, connecting to multiple SRDriver devices via WebSocket and sending commands to them. It mirrors the functionality of the webapp but runs on the ESP32 with LVGL UI.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│              CrowPanel (Controller Device)                │
│                                                           │
│  ┌─────────────────────────────────────────────────┐   │
│  │  LVGL UI (Touch Screen)                          │   │
│  │  - Main screen with "Devices" button             │   │
│  │  - Device management screen                      │   │
│  │  - IP input, connect button                      │   │
│  │  - List of connected devices                     │   │
│  │  - Per-device controls (brightness, etc.)        │   │
│  └─────────────────────────────────────────────────┘   │
│                        │                                 │
│                        ▼                                 │
│  ┌─────────────────────────────────────────────────┐   │
│  │  DeviceManager (Singleton)                      │   │
│  │  - Manages list of connected devices             │   │
│  │  - Tracks connection status                      │   │
│  │  - Routes commands to specific devices           │   │
│  │  - Handles auto-reconnect logic                  │   │
│  └─────────────────────────────────────────────────┘   │
│                        │                                 │
│        ┌───────────────┼───────────────┐               │
│        ▼               ▼               ▼               │
│  ┌─────────┐   ┌─────────┐   ┌─────────┐           │
│  │ Client 1 │   │ Client 2 │   │ Client 3 │           │
│  │ (Device1)│   │ (Device2)│   │ (Device3)│           │
│  └─────────┘   └─────────┘   └─────────┘           │
│        │               │               │               │
└────────┼───────────────┼───────────────┼───────────────┘
         │               │               │
         ▼               ▼               ▼
    ┌─────────┐   ┌─────────┐   ┌─────────┐
    │Device 1 │   │Device 2 │   │Device 3 │
    │(Server) │   │(Server)  │   │(Server) │
    └─────────┘   └─────────┘   └─────────┘
```

## Components

### 1. SRWebSocketClient

**Purpose**: Wrapper around `WebSocketsClient` to manage a single device connection.

**File**: `src/hal/network/WebSocketClient.h/cpp`

**Key Features**:
- Connect/disconnect to remote device by IP address (port 8080)
- Send JSON commands (brightness, effects, etc.)
- Receive status updates from device
- Auto-reconnect with exponential backoff
- Connection state tracking

**API**:
```cpp
class SRWebSocketClient {
public:
    SRWebSocketClient(const String& ipAddress);
    ~SRWebSocketClient();
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;
    ConnectionState getState() const;
    
    // Command sending
    bool sendCommand(const String& jsonCommand);
    bool sendBrightness(uint8_t brightness);
    
    // Status
    String getIP() const;
    String getLastStatus() const;  // Last status message received
    unsigned long getLastActivity() const;
    
    // Auto-reconnect
    void setAutoReconnect(bool enable);
    bool getAutoReconnect() const;
    void resetReconnectAttempts();  // Manual trigger to retry
    
    // Must call in loop()
    void update();
    
private:
    // Auto-reconnect logic (self-contained)
    bool shouldAutoReconnect() const;
    void tryReconnect();
    void calculateNextReconnectDelay();
};
```

**Connection States**:
- `DISCONNECTED` - Not connected, not attempting
- `CONNECTING` - Connection attempt in progress
- `CONNECTED` - Connected and ready
- `RECONNECTING` - Auto-reconnect attempt in progress

### 2. DeviceManager

**Purpose**: Singleton that manages multiple device connections.

**File**: `src/hal/network/DeviceManager.h/cpp`

**Key Features**:
- Store list of connected devices (max 10, configurable)
- Add/remove devices by IP address
- Route commands to specific devices
- Track connection status for all devices
- Auto-reconnect management for all devices
- Device identification and naming

**API**:
```cpp
class DeviceManager {
public:
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
    size_t getDeviceCount() const;
    size_t getConnectedCount() const;
    bool isDeviceConnected(const String& ipAddress) const;
    
    // Status
    String getDeviceListJSON() const;  // For UI display
    String getDeviceStatus(const String& ipAddress) const;
    
    // Must call in loop()
    void update();
    
private:
    struct DeviceInfo {
        String ipAddress;
        String displayName;  // "SRDriver 1", "SRDriver 2", etc.
        SRWebSocketClient* client;
        unsigned long lastActivity;
        bool autoReconnect;
    };
    
    std::vector<DeviceInfo> _devices;
    String generateDisplayName() const;  // Auto-generate "SRDriver N"
    void checkAndReconnectDevices();  // Clean auto-reconnect logic
};
```

### 3. LVGL UI Integration

**Purpose**: User interface for device management on CrowPanel.

**File**: `src/lvglui.h/cpp` (updates)

**UI Elements**:
- **Main Screen**: "Devices" button (3rd button in 2x2 grid)
- **Device Management Screen** (new screen):
  - IP address input field (text area)
  - "Connect" button
  - List of connected devices:
    - IP address
    - Display name
    - Connection status indicator (green/red)
    - Last activity timestamp
    - Brightness slider (per device)
    - "Disconnect" button (per device)
  - "Back" button to return to main screen

**UI Flow**:
```
Main Screen
  └─> [Devices Button] clicked
       └─> Device Management Screen
            ├─> IP Input + Connect Button
            ├─> Connected Devices List (scrollable)
            │    └─> Each device shows:
            │         - IP: 192.168.1.100
            │         - Name: SRDriver 1
            │         - Status: ● (green/red)
            │         - Brightness: [====|----] 50%
            │         - [Disconnect] button
            └─> [Back] Button → Main Screen
```

## Configuration

### Constants

```cpp
// In WebSocketClient.h
#define MAX_DEVICES 10                    // Maximum number of devices
#define RECONNECT_DELAY_MS 5000           // Initial reconnect delay (5 seconds)
#define MAX_RECONNECT_ATTEMPTS 10         // Stop after 10 failures
#define RECONNECT_BACKOFF_MULTIPLIER 2.0f  // Exponential backoff multiplier
#define RECONNECT_JITTER_MS 1000          // Random jitter range (±500ms)
```

### Auto-Reconnect Behavior

**Initial Delay**: 5 seconds after disconnect

**Exponential Backoff**: 
- Attempt 1: 5 seconds
- Attempt 2: 10 seconds (5 × 2)
- Attempt 3: 20 seconds (10 × 2)
- Attempt 4: 40 seconds (20 × 2)
- ... up to MAX_RECONNECT_ATTEMPTS

**Random Jitter**: ±500ms added to each delay to prevent thundering herd

**Max Attempts**: After 10 failed attempts, auto-reconnect stops. User can manually trigger retry via UI button.

**Implementation Pattern**:
```cpp
bool SRWebSocketClient::shouldAutoReconnect() const {
    if (!_autoReconnect) return false;
    if (_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) return false;
    
    unsigned long elapsed = millis() - _lastDisconnectTime;
    return elapsed >= _nextReconnectDelay;
}

void SRWebSocketClient::tryReconnect() {
    if (!shouldAutoReconnect()) return;
    
    _state = RECONNECTING;
    if (connect()) {
        _reconnectAttempts = 0;  // Reset on success
    } else {
        _reconnectAttempts++;
        calculateNextReconnectDelay();  // Exponential backoff + jitter
    }
}
```

## Command Protocol

Uses the same JSON format as webapp and existing WebSocketServer:

```json
// Brightness command
{"t": "brightness", "brightness": 50}

// Effect command
{"t": "effect", "effect": "solid", "color": "#FF0000"}

// Status request (future)
{"t": "status"}
```

**Status Response** (from devices):
```json
{
  "type": "status",
  "timestamp": 12345,
  "brightness": 50,
  "effect": "solid",
  ...
}
```

## Data Storage

**Current**: In-memory only (no persistence)

**Future**: Persist device list to SD card/preferences
- Store IP addresses
- Store display names
- Auto-reconnect on boot

## Update Frequency

**DeviceManager::update()**: Called every loop iteration in `main.cpp`

**SRWebSocketClient::update()**: Called by DeviceManager for each device

**WebSocketsClient**: Handles internal throttling automatically

## Error Handling

**Connection Failures**:
- Log error via LogManager
- Return `false` from `connect()` or `sendCommand()`
- Update connection state
- Trigger auto-reconnect if enabled

**Command Failures**:
- Return `false` from `sendCommand()`
- No command queuing (fail fast)
- UI can poll status to check if device is connected

**UI Feedback**:
- Connection status indicator (green/red dot)
- Last activity timestamp
- Error messages in logs (UI polling for now, no callbacks)

## Device Identification

**Primary ID**: IP address (unique on network)

**Display Name**: Auto-generated "SRDriver 1", "SRDriver 2", etc.
- Generated when device is added
- Can be customized later (future feature)

**Device Lookup**: By IP address (primary key)

## Status Updates

**Receiving**: Devices broadcast status updates to all connected clients
- Captured in `SRWebSocketClient::onMessage()` callback
- Stored as `_lastStatusMessage` string
- Parsed JSON available via `getLastStatus()`

**Requesting**: Future feature - send `{"t": "status"}` command to request status

**UI Updates**: UI polls `getLastStatus()` periodically to update display

## Thread Safety

**Current**: All operations on main thread (FreeRTOS task)
- No thread safety needed initially
- All WebSocket operations are non-blocking

**Future**: If moved to separate task, add mutex protection

## Memory Considerations

**Per Device**:
- SRWebSocketClient instance: ~200-300 bytes
- DeviceInfo struct: ~100 bytes
- WebSocketsClient internal: ~1-2 KB

**Total for 10 devices**: ~15-20 KB

**Optimization**: Use PSRAM if available for client buffers

## Implementation Phases

### Phase 1: Basic Connection ✅ (Starting Here)
- [x] SRWebSocketClient class (connect/disconnect/send)
- [x] DeviceManager class (single device)
- [x] Basic UI (IP input, connect button, status)

### Phase 2: Multiple Devices
- [ ] DeviceManager: support multiple devices
- [ ] UI: list of connected devices
- [ ] UI: per-device controls

### Phase 3: Advanced Features
- [ ] Device naming/customization
- [ ] Status message parsing
- [ ] Request status command
- [ ] Error handling improvements
- [ ] Device persistence (SD card)

## File Structure

```
src/hal/network/
  ├── WebSocketServer.h/cpp    (existing - server side)
  ├── WebSocketClient.h/cpp    (new - client side)
  └── DeviceManager.h/cpp       (new - multi-device management)

src/
  ├── lvglui.h/cpp             (update - add device UI)
  └── main.cpp                 (update - call DeviceManager::update())
```

## Usage Example

```cpp
// In main.cpp setup()
#if PLATFORM_CROW_PANEL
// DeviceManager is singleton, automatically available
#endif

// In main.cpp loop()
#if PLATFORM_CROW_PANEL
DeviceManager::getInstance().update();
#endif

// In LVGL UI event handler
void connectDeviceHandler(lv_event_t* e) {
    const char* ip = lv_textarea_get_text(lvgl_deviceIPInput);
    if (DeviceManager::getInstance().connectDevice(String(ip))) {
        updateDeviceList();  // Refresh UI
    }
}

// Send brightness to specific device
void deviceBrightnessHandler(lv_event_t* e, const String& ipAddress) {
    int32_t brightness = lv_slider_get_value(e->target);
    DeviceManager::getInstance().sendBrightnessToDevice(ipAddress, brightness);
}
```

## Testing Strategy

1. **Single Device**: Connect to one device, send brightness command
2. **Multiple Devices**: Connect to 2-3 devices, verify independent control
3. **Auto-Reconnect**: Disconnect device, verify reconnection attempts
4. **Max Devices**: Try connecting 11th device, verify rejection
5. **Status Updates**: Verify status messages are received and stored
6. **UI Integration**: Test full UI flow (connect, control, disconnect)

---

**Last Updated**: 2024-12-19
**Status**: Design Complete - Ready for Implementation

