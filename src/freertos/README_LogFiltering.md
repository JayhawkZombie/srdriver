# Log Filtering System

The SRDriver logging system now supports advanced filtering capabilities, allowing you to focus on specific components and filter out old logs.

## Features

### 1. Component-Based Filtering
- Filter logs by component (e.g., "WiFiManager", "BLEManager", "PatternManager")
- Runtime configurable filtering
- Multiple components can be filtered simultaneously

### 2. Timestamp-Based Filtering
- Filter out old logs to show only recent activity
- Set a minimum timestamp to ignore logs before a certain time
- "New logs only" mode to show only logs generated after activation

### 3. Backward Compatibility
- All existing `LOG_DEBUG(msg)` calls continue to work unchanged
- Gradual migration to component-aware logging
- No breaking changes to existing code

## Usage Examples

### Basic Component-Aware Logging

```cpp
// Old way (still works):
LOG_DEBUG("Connection attempt started");

// New way (with component identification):
LOG_DEBUG_COMPONENT("WiFiManager", "Connection attempt started");
LOG_INFO_COMPONENT("WiFiManager", "WiFi connected successfully");
LOG_ERROR_COMPONENT("BLEManager", "BLE initialization failed");
```

### Component Filtering

```cpp
// Show only WiFiManager logs
std::vector<String> wifiOnly = {"WiFiManager"};
LOG_SET_COMPONENT_FILTER(wifiOnly);

// Show multiple components
std::vector<String> networkComponents = {"WiFiManager", "BLEManager"};
LOG_SET_COMPONENT_FILTER(networkComponents);

// Show all components (disable filtering)
LOG_ENABLE_ALL_COMPONENTS();
```

### Timestamp Filtering

```cpp
// Show only new logs (from this point forward)
LOG_SET_NEW_LOGS_ONLY();

// Set specific timestamp filter
uint32_t timestamp = millis() - 5000; // 5 seconds ago
logger.setTimestampFilter(timestamp);

// Disable timestamp filtering
LOG_DISABLE_TIMESTAMP_FILTER();
```

### Runtime Filter Management

```cpp
// Add/remove components dynamically
LOG_ADD_COMPONENT("PatternManager");
LOG_REMOVE_COMPONENT("BLEManager");

// Check current filtering status
if (logger.isComponentFilteringEnabled()) {
    auto allowed = logger.getAllowedComponents();
    // Process allowed components
}
```

## Log Output Format

### Legacy Format (no component)
```
[DEBUG] 12345: Connection attempt started
```

### New Format (with component)
```
[WiFiManager] [DEBUG] 12345: Connection attempt started
```

### SD Card Log Format
```
[12345] [WiFiManager] DEBUG: Connection attempt started
```

## Migration Strategy

### Phase 1: Update High-Priority Components
Start with components that generate the most logs:
- WiFiManager
- BLEManager  
- PatternManager

### Phase 2: Gradual Migration
Replace existing logging calls:
```cpp
// Before:
LOG_DEBUG("WiFi connection started");

// After:
LOG_DEBUG_COMPONENT("WiFiManager", "WiFi connection started");
```

### Phase 3: Add Filtering Controls
Integrate filtering controls into BLE commands or web interface.

## BLE Command Integration

Example BLE commands for log filtering:

```
LOG_FILTER_WIFI_ONLY      - Show only WiFiManager logs
LOG_FILTER_NETWORK        - Show WiFiManager and BLEManager logs  
LOG_FILTER_ALL            - Show all logs (disable filtering)
LOG_FILTER_NEW_ONLY       - Show only new logs
LOG_ADD_COMPONENT:PatternManager
LOG_REMOVE_COMPONENT:BLEManager
```

## Performance Considerations

- **Minimal Overhead**: Component filtering adds only a simple string comparison
- **Memory Efficient**: Uses fixed-size arrays for component names
- **Fast Filtering**: Early return when filters don't match
- **No Impact on Legacy Code**: Existing logging calls unchanged

## Advanced Usage

### Custom Filtering Logic
```cpp
// Check if specific component is allowed
if (logger.isComponentFilteringEnabled()) {
    auto allowed = logger.getAllowedComponents();
    bool wifiAllowed = std::find(allowed.begin(), allowed.end(), "WiFiManager") != allowed.end();
}
```

### Conditional Logging
```cpp
// Only log if component is in filter
if (logger.isComponentFilteringEnabled()) {
    auto allowed = logger.getAllowedComponents();
    if (std::find(allowed.begin(), allowed.end(), "WiFiManager") != allowed.end()) {
        LOG_DEBUG_COMPONENT("WiFiManager", "Detailed debug info");
    }
}
```

## Troubleshooting

### Common Issues

1. **Component name too long**: Component names are limited to 31 characters
2. **Case sensitivity**: Component names are case-sensitive
3. **Empty component**: Legacy logs (no component) are always shown unless timestamp filtered

### Debug Filtering
```cpp
// Check current filter status
Serial.println("Component filtering: " + String(logger.isComponentFilteringEnabled()));
Serial.println("Timestamp filtering: " + String(logger.isTimestampFilteringEnabled()));
Serial.println("Min timestamp: " + String(logger.getMinTimestamp()));
```
