#include "lvgl_devices.h"
#include "lvglui.h"  // For lvgl_screen access
#include "hal/network/DeviceManager.h"
#include "freertos/LogManager.h"
#include <ArduinoJson.h>

#if PLATFORM_CROW_PANEL

// Device management screen objects
lv_obj_t* lvgl_devicesScreen = nullptr;
lv_obj_t* lvgl_deviceIPInput = nullptr;
lv_obj_t* lvgl_deviceConnectBtn = nullptr;
lv_obj_t* lvgl_deviceList = nullptr;
lv_obj_t* lvgl_deviceStatusLabel = nullptr;

// Map to store device UI elements (IP -> container object)
#include <map>
std::map<String, lv_obj_t*> deviceUIContainers;

// Helper function to get IP address from a device container
static String getIPFromContainer(lv_obj_t* obj) {
    // Walk up the parent chain to find the device container
    lv_obj_t* current = obj;
    while (current != nullptr) {
        // Check if this container is in our map
        for (const auto& pair : deviceUIContainers) {
            if (pair.second == current) {
                return pair.first;
            }
        }
        current = lv_obj_get_parent(current);
    }
    return String("");  // Not found
}

// Forward declarations
static void createDeviceManagementScreen();
static void deviceConnectBtnEventHandler(lv_event_t* e);
static void deviceScreenBackBtnEventHandler(lv_event_t* e);
static void deviceBrightnessSliderEventHandler(lv_event_t* e);
static void deviceDisconnectBtnEventHandler(lv_event_t* e);
static void createDeviceListItem(const String& ipAddress, const String& displayName, bool isConnected);
static void removeDeviceListItem(const String& ipAddress);

void showDeviceManagement() {
    Serial.println("[LVGL] Showing device management screen...");
    
    // Create screen if it doesn't exist
    if (lvgl_devicesScreen == nullptr) {
        createDeviceManagementScreen();
    }
    
    // Load the device management screen
    lv_scr_load(lvgl_devicesScreen);
    
    // Update device list
    updateDeviceList();
    
    Serial.println("[LVGL] Device management screen shown");
}

void hideDeviceManagement() {
    if (lvgl_devicesScreen == nullptr) {
        return;
    }
    
    // Return to main screen
    if (lvgl_screen != nullptr) {
        lv_scr_load(lvgl_screen);
        Serial.println("[LVGL] Returned to main screen");
    }
}

bool isDeviceManagementShown() {
    return lvgl_devicesScreen != nullptr && lv_scr_act() == lvgl_devicesScreen;
}

static void createDeviceManagementScreen() {
    Serial.println("[LVGL] Creating device management screen...");
    
    // Create new screen
    lvgl_devicesScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(lvgl_devicesScreen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lvgl_devicesScreen, LV_OPA_COVER, 0);
    
    // Get screen dimensions
    lv_coord_t screenWidth = lv_obj_get_width(lvgl_screen);
    lv_coord_t screenHeight = lv_obj_get_height(lvgl_screen);
    
    // Create header with title and back button
    lv_obj_t* header = lv_obj_create(lvgl_devicesScreen);
    lv_obj_set_size(header, LV_PCT(100), 60);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_border_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(header, 10, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Device Manager");
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Back button
    lv_obj_t* backBtn = lv_btn_create(header);
    lv_obj_set_size(backBtn, 80, 40);
    lv_obj_align(backBtn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x808080), 0);
    lv_obj_add_event_cb(backBtn, deviceScreenBackBtnEventHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* backBtnLabel = lv_label_create(backBtn);
    lv_label_set_text(backBtnLabel, "Back");
    lv_obj_center(backBtnLabel);
    
    // Connection section (IP input + Connect button)
    lv_obj_t* connectSection = lv_obj_create(lvgl_devicesScreen);
    lv_obj_set_size(connectSection, LV_PCT(100), 80);
    lv_obj_set_pos(connectSection, 0, 60);
    lv_obj_set_style_bg_opa(connectSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(connectSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(connectSection, 10, 0);
    
    // IP input label
    lv_obj_t* ipLabel = lv_label_create(connectSection);
    lv_label_set_text(ipLabel, "IP Address:");
    lv_obj_align(ipLabel, LV_ALIGN_TOP_LEFT, 10, 5);
    
    // IP input textarea
    lvgl_deviceIPInput = lv_textarea_create(connectSection);
    lv_obj_set_size(lvgl_deviceIPInput, 200, 40);
    lv_obj_align(lvgl_deviceIPInput, LV_ALIGN_TOP_LEFT, 10, 25);
    lv_textarea_set_placeholder_text(lvgl_deviceIPInput, "192.168.1.100");
    lv_textarea_set_max_length(lvgl_deviceIPInput, 15);
    lv_textarea_set_one_line(lvgl_deviceIPInput, true);
    
    // Connect button
    lvgl_deviceConnectBtn = lv_btn_create(connectSection);
    lv_obj_set_size(lvgl_deviceConnectBtn, 100, 40);
    lv_obj_align(lvgl_deviceConnectBtn, LV_ALIGN_TOP_LEFT, 220, 25);
    lv_obj_set_style_bg_color(lvgl_deviceConnectBtn, lv_color_hex(0x4CAF50), 0);  // Green
    lv_obj_add_event_cb(lvgl_deviceConnectBtn, deviceConnectBtnEventHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* connectBtnLabel = lv_label_create(lvgl_deviceConnectBtn);
    lv_label_set_text(connectBtnLabel, "Connect");
    lv_obj_center(connectBtnLabel);
    
    // Device list (scrollable container)
    lvgl_deviceList = lv_obj_create(lvgl_devicesScreen);
    lv_obj_set_size(lvgl_deviceList, LV_PCT(100), screenHeight - 200);  // Leave room for header, connect section, and status
    lv_obj_set_pos(lvgl_deviceList, 0, 140);
    lv_obj_set_style_bg_opa(lvgl_deviceList, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(lvgl_deviceList, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(lvgl_deviceList, 10, 0);
    lv_obj_set_flex_flow(lvgl_deviceList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lvgl_deviceList, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(lvgl_deviceList, LV_LAYOUT_FLEX);
    lv_obj_set_scroll_dir(lvgl_deviceList, LV_DIR_VER);
    lv_obj_clear_flag(lvgl_deviceList, LV_OBJ_FLAG_SCROLL_ELASTIC);  // No elastic scrolling
    
    // Status label at bottom
    lvgl_deviceStatusLabel = lv_label_create(lvgl_devicesScreen);
    lv_obj_set_size(lvgl_deviceStatusLabel, LV_PCT(100), 30);
    lv_obj_align(lvgl_deviceStatusLabel, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_align(lvgl_deviceStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lvgl_deviceStatusLabel, "No devices connected");
    
    Serial.println("[LVGL] Device management screen created");
}

static void deviceConnectBtnEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (lvgl_deviceIPInput == nullptr) {
            Serial.println("[LVGL] ERROR: IP input not found");
            return;
        }
        
        const char* ipText = lv_textarea_get_text(lvgl_deviceIPInput);
        if (ipText == nullptr || strlen(ipText) == 0) {
            Serial.println("[LVGL] No IP address entered");
            return;
        }
        
        String ipAddress = String(ipText);
        Serial.printf("[LVGL] Attempting to connect to device: %s\n", ipAddress.c_str());
        
        // Attempt connection
        if (DeviceManager::getInstance().connectDevice(ipAddress)) {
            Serial.printf("[LVGL] Successfully initiated connection to %s\n", ipAddress.c_str());
            // Clear IP input
            lv_textarea_set_text(lvgl_deviceIPInput, "");
            // Update device list
            updateDeviceList();
        } else {
            Serial.printf("[LVGL] Failed to connect to %s\n", ipAddress.c_str());
            // Could show error message here
        }
    }
}

static void deviceScreenBackBtnEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[LVGL] Back button clicked - returning to main screen");
        hideDeviceManagement();
    }
}

static void deviceBrightnessSliderEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* slider = lv_event_get_target(e);
        int32_t brightness = lv_slider_get_value(slider);
        
        // Get IP address by finding the container in our map
        String ip = getIPFromContainer(slider);
        if (ip.length() > 0) {
            Serial.printf("[LVGL] Setting brightness for %s to %d\n", ip.c_str(), brightness);
            DeviceManager::getInstance().sendBrightnessToDevice(ip, brightness);
        } else {
            Serial.println("[LVGL] ERROR: Could not find IP address for brightness slider");
        }
    }
}

static void deviceDisconnectBtnEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* btn = lv_event_get_target(e);
        // Get IP address by finding the container in our map
        String ip = getIPFromContainer(btn);
        if (ip.length() > 0) {
            Serial.printf("[LVGL] Disconnecting device: %s\n", ip.c_str());
            DeviceManager::getInstance().disconnectDevice(ip);
            // Update device list
            updateDeviceList();
        } else {
            Serial.println("[LVGL] ERROR: Could not find IP address for disconnect button");
        }
    }
}

static void createDeviceListItem(const String& ipAddress, const String& displayName, bool isConnected) {
    if (lvgl_deviceList == nullptr) {
        return;
    }
    
    // Check if device already exists in UI
    if (deviceUIContainers.find(ipAddress) != deviceUIContainers.end()) {
        // Device already in list, just update it
        return;
    }
    
    Serial.printf("[LVGL] Creating UI item for device: %s (%s)\n", ipAddress.c_str(), displayName.c_str());
    
    // Create device container
    lv_obj_t* deviceContainer = lv_obj_create(lvgl_deviceList);
    lv_obj_set_size(deviceContainer, LV_PCT(95), 120);
    lv_obj_set_style_bg_color(deviceContainer, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(deviceContainer, 2, 0);
    lv_obj_set_style_border_color(deviceContainer, lv_color_black(), 0);
    lv_obj_set_style_radius(deviceContainer, 5, 0);
    lv_obj_set_style_pad_all(deviceContainer, 10, 0);
    lv_obj_set_flex_flow(deviceContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(deviceContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(deviceContainer, LV_LAYOUT_FLEX);
    
    // Store container in map
    deviceUIContainers[ipAddress] = deviceContainer;
    
    // Device info row (IP + status indicator)
    lv_obj_t* infoRow = lv_obj_create(deviceContainer);
    lv_obj_set_size(infoRow, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(infoRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(infoRow, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(infoRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_layout(infoRow, LV_LAYOUT_FLEX);
    
    // Status indicator (colored circle)
    lv_obj_t* statusIndicator = lv_obj_create(infoRow);
    lv_obj_set_size(statusIndicator, 20, 20);
    lv_obj_set_style_bg_color(statusIndicator, isConnected ? lv_color_hex(0x4CAF50) : lv_color_hex(0xF44336), 0);
    lv_obj_set_style_bg_opa(statusIndicator, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(statusIndicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_opa(statusIndicator, LV_OPA_TRANSP, 0);
    
    // Device name/IP label
    lv_obj_t* deviceLabel = lv_label_create(infoRow);
    char labelText[128];
    snprintf(labelText, sizeof(labelText), "%s\n%s", displayName.c_str(), ipAddress.c_str());
    lv_label_set_text(deviceLabel, labelText);
    lv_obj_set_style_text_align(deviceLabel, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_flex_grow(deviceLabel, 1);
    
    // Brightness slider row
    lv_obj_t* sliderRow = lv_obj_create(deviceContainer);
    lv_obj_set_size(sliderRow, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(sliderRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(sliderRow, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(sliderRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_layout(sliderRow, LV_LAYOUT_FLEX);
    
    // Brightness label
    lv_obj_t* brightnessLabel = lv_label_create(sliderRow);
    lv_label_set_text(brightnessLabel, "Brightness:");
    lv_obj_set_width(brightnessLabel, 100);
    
    // Brightness slider
    lv_obj_t* brightnessSlider = lv_slider_create(sliderRow);
    lv_obj_set_size(brightnessSlider, 200, 20);
    lv_slider_set_range(brightnessSlider, 0, 100);
    lv_slider_set_value(brightnessSlider, 50, LV_ANIM_OFF);
    // No need to store IP - we'll look it up from the container hierarchy
    lv_obj_add_event_cb(brightnessSlider, deviceBrightnessSliderEventHandler, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_flex_grow(brightnessSlider, 1);
    
    // Brightness value label
    lv_obj_t* brightnessValueLabel = lv_label_create(sliderRow);
    lv_label_set_text(brightnessValueLabel, "50%");
    lv_obj_set_width(brightnessValueLabel, 50);
    // Store reference to value label in slider user data (we'll need to update it)
    // For now, just show static value
    
    // Disconnect button
    lv_obj_t* disconnectBtn = lv_btn_create(deviceContainer);
    lv_obj_set_size(disconnectBtn, 120, 35);
    lv_obj_set_style_bg_color(disconnectBtn, lv_color_hex(0xF44336), 0);  // Red
    // No need to store IP - we'll look it up from the container hierarchy
    lv_obj_add_event_cb(disconnectBtn, deviceDisconnectBtnEventHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* disconnectBtnLabel = lv_label_create(disconnectBtn);
    lv_label_set_text(disconnectBtnLabel, "Disconnect");
    lv_obj_center(disconnectBtnLabel);
}

static void removeDeviceListItem(const String& ipAddress) {
    auto it = deviceUIContainers.find(ipAddress);
    if (it != deviceUIContainers.end()) {
        Serial.printf("[LVGL] Removing UI item for device: %s\n", ipAddress.c_str());
        // Delete the container (LVGL will automatically delete all child objects)
        lv_obj_del(it->second);
        deviceUIContainers.erase(it);
        // No memory to free - we're using the map lookup approach instead of malloc
    }
}

void updateDeviceList() {
    if (lvgl_deviceList == nullptr) {
        return;
    }
    
    // Get all devices from DeviceManager
    auto& deviceMgr = DeviceManager::getInstance();
    size_t deviceCount = deviceMgr.getDeviceCount();
    size_t connectedCount = deviceMgr.getConnectedCount();
    
    // Update status label
    if (lvgl_deviceStatusLabel != nullptr) {
        char statusText[64];
        snprintf(statusText, sizeof(statusText), "%d device(s) connected / %d total", 
                 connectedCount, deviceCount);
        lv_label_set_text(lvgl_deviceStatusLabel, statusText);
    }
    
    // Get device list from DeviceManager (we'll need to add a method to iterate devices)
    // For now, we'll use the JSON method
    String deviceListJSON = deviceMgr.getDeviceListJSON();
    
    // Parse JSON to get devices
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, deviceListJSON);
    
    if (error) {
        Serial.printf("[LVGL] Failed to parse device list JSON: %s\n", error.c_str());
        return;
    }
    
    JsonArray devices = doc["devices"].as<JsonArray>();
    
    // Track which devices are still in the manager
    std::map<String, bool> currentDevices;
    
    // Update/create device list items
    for (JsonObject device : devices) {
        String ip = device["ip"].as<String>();
        String name = device["name"].as<String>();
        bool connected = device["connected"].as<bool>();
        
        currentDevices[ip] = true;
        
        // Create or update device item
        createDeviceListItem(ip, name, connected);
    }
    
    // Remove devices that are no longer in the manager
    for (auto it = deviceUIContainers.begin(); it != deviceUIContainers.end();) {
        if (currentDevices.find(it->first) == currentDevices.end()) {
            // Device no longer exists, remove from UI
            removeDeviceListItem(it->first);
            it = deviceUIContainers.erase(it);
        } else {
            ++it;
        }
    }
    
    Serial.printf("[LVGL] Device list updated: %d devices\n", deviceCount);
}

#endif // PLATFORM_CROW_PANEL

