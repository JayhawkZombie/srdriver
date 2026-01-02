#include "lvgl_devices.h"
#include "lvglui.h"  // For lvgl_screen access
#include "hal/network/DeviceManager.h"
#include "hal/SDCardController.h"
#include "freertos/LogManager.h"
#include <ArduinoJson.h>
#include <vector>

#if PLATFORM_CROW_PANEL

// Device management screen objects
lv_obj_t* lvgl_devicesScreen = nullptr;
lv_obj_t* lvgl_deviceIPPrefixInput = nullptr;  // IP prefix input (e.g., "192.168.1")
lv_obj_t* lvgl_deviceIPLastInput = nullptr;    // Last octet input (e.g., "163")
lv_obj_t* lvgl_deviceConnectBtn = nullptr;
lv_obj_t* lvgl_deviceList = nullptr;
lv_obj_t* lvgl_deviceStatusLabel = nullptr;
lv_obj_t* lvgl_keyboard = nullptr;
lv_obj_t* lvgl_deviceDropdown = nullptr;
lv_obj_t* lvgl_deviceDropdownConnectBtn = nullptr;

// Map to store device UI elements (IP -> container object)
#include <map>
std::map<String, lv_obj_t*> deviceUIContainers;

// Storage for previously connected devices (for dropdown)
std::vector<String> previouslyConnectedDevices;

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
static void deviceNextEffectBtnEventHandler(lv_event_t* e);
static void textareaFocusedEventHandler(lv_event_t* e);
static void textareaDefocusedEventHandler(lv_event_t* e);
static void createDeviceListItem(const String& ipAddress, const String& displayName, bool isConnected);
static void removeDeviceListItem(const String& ipAddress);
static void deviceDropdownConnectBtnEventHandler(lv_event_t* e);
static void refreshDeviceDropdown();
static void updateDropdownConnectButtonState();

// Device storage functions
static bool loadPreviouslyConnectedDevices();
static bool savePreviouslyConnectedDevices();
static void addPreviouslyConnectedDevice(const String& ipAddress);

void showDeviceManagement() {
    LOG_DEBUG_COMPONENT("LVGL", "Showing device management screen...");
    
    // Create screen if it doesn't exist
    if (lvgl_devicesScreen == nullptr) {
        createDeviceManagementScreen();
    }
    
    // Load the device management screen
    lv_scr_load(lvgl_devicesScreen);
    
    // Refresh dropdown and update device list
    refreshDeviceDropdown();
    updateDeviceList();
    
    LOG_DEBUG_COMPONENT("LVGL", "Device management screen shown");
}

void hideDeviceManagement() {
    if (lvgl_devicesScreen == nullptr) {
        return;
    }
    
        // Return to main screen
        if (lvgl_screen != nullptr) {
            lv_scr_load(lvgl_screen);
            LOG_DEBUG_COMPONENT("LVGL", "Returned to main screen");
        }
}

bool isDeviceManagementShown() {
    return lvgl_devicesScreen != nullptr && lv_scr_act() == lvgl_devicesScreen;
}

static void createDeviceManagementScreen() {
    LOG_DEBUG_COMPONENT("LVGL", "Creating device management screen...");
    
    // Create new screen with flex layout
    lvgl_devicesScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(lvgl_devicesScreen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lvgl_devicesScreen, LV_OPA_COVER, 0);
    // Make screen a flex container (column layout)
    lv_obj_set_flex_flow(lvgl_devicesScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lvgl_devicesScreen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(lvgl_devicesScreen, LV_LAYOUT_FLEX);
    
    // Create header with title and back button (fixed height)
    lv_obj_t* header = lv_obj_create(lvgl_devicesScreen);
    lv_obj_set_size(header, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_border_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(header, 10, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
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
    
    // Connection section (IP input on left, Previous dropdown on right, 50/50 split)
    // Auto-sized based on content - no fixed height
    lv_obj_t* connectSection = lv_obj_create(lvgl_devicesScreen);
    lv_obj_set_width(connectSection, LV_PCT(100));
    lv_obj_set_height(connectSection, LV_SIZE_CONTENT);  // Auto-size based on content
    lv_obj_set_style_bg_opa(connectSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(connectSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(connectSection, 8, 0);  // More padding for breathing room
    lv_obj_set_style_pad_row(connectSection, 6, 0);  // Vertical spacing between children
    lv_obj_set_flex_flow(connectSection, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(connectSection, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(connectSection, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(connectSection, LV_OBJ_FLAG_SCROLLABLE);  // Prevent scrolling
    
    // Left side: IP input section (50% width) - flex column layout
    lv_obj_t* ipSection = lv_obj_create(connectSection);
    lv_obj_set_width(ipSection, LV_PCT(48));
    lv_obj_set_height(ipSection, LV_SIZE_CONTENT);  // Auto-size
    lv_obj_set_style_bg_opa(ipSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(ipSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(ipSection, 0, 0);
    lv_obj_set_flex_flow(ipSection, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ipSection, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(ipSection, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(ipSection, LV_OBJ_FLAG_SCROLLABLE);
    
    // IP input label
    lv_obj_t* ipLabel = lv_label_create(ipSection);
    lv_label_set_text(ipLabel, "IP Address:");
    lv_obj_set_width(ipLabel, LV_PCT(100));
    
    // IP input row container - flex row layout
    lv_obj_t* ipInputRow = lv_obj_create(ipSection);
    lv_obj_set_width(ipInputRow, LV_PCT(100));
    lv_obj_set_height(ipInputRow, 40);  // Normal height for inputs
    lv_obj_set_style_bg_opa(ipInputRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(ipInputRow, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(ipInputRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ipInputRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_layout(ipInputRow, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(ipInputRow, LV_OBJ_FLAG_SCROLLABLE);
    
    // IP prefix input (e.g., "192.168.1") - reduced width, taller to match button visual size
    lvgl_deviceIPPrefixInput = lv_textarea_create(ipInputRow);
    lv_obj_set_size(lvgl_deviceIPPrefixInput, 120, 40);  // Fixed width instead of percentage, height 40px
    lv_textarea_set_text(lvgl_deviceIPPrefixInput, "192.168.1");  // Default value
    lv_textarea_set_placeholder_text(lvgl_deviceIPPrefixInput, "192.168.1");
    lv_textarea_set_max_length(lvgl_deviceIPPrefixInput, 15);
    lv_textarea_set_one_line(lvgl_deviceIPPrefixInput, true);
    // Add padding to prevent cut-off and make textarea taller visually
    lv_obj_set_style_pad_top(lvgl_deviceIPPrefixInput, 6, 0);
    lv_obj_set_style_pad_bottom(lvgl_deviceIPPrefixInput, 6, 0);
    lv_obj_set_style_pad_left(lvgl_deviceIPPrefixInput, 4, 0);
    lv_obj_set_style_pad_right(lvgl_deviceIPPrefixInput, 4, 0);
    // Add event handlers for keyboard show/hide
    lv_obj_add_event_cb(lvgl_deviceIPPrefixInput, textareaFocusedEventHandler, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(lvgl_deviceIPPrefixInput, textareaDefocusedEventHandler, LV_EVENT_DEFOCUSED, nullptr);
    
    // Dot separator label
    lv_obj_t* dotLabel = lv_label_create(ipInputRow);
    lv_label_set_text(dotLabel, ".");
    lv_obj_set_style_pad_left(dotLabel, 2, 0);
    lv_obj_set_style_pad_right(dotLabel, 2, 0);
    
    // Last octet input (e.g., "163") - reduced width, taller to match button visual size
    lvgl_deviceIPLastInput = lv_textarea_create(ipInputRow);
    lv_obj_set_size(lvgl_deviceIPLastInput, 50, 40);  // Fixed width instead of percentage, height 40px
    lv_textarea_set_placeholder_text(lvgl_deviceIPLastInput, "163");
    lv_textarea_set_max_length(lvgl_deviceIPLastInput, 3);
    lv_textarea_set_one_line(lvgl_deviceIPLastInput, true);
    // Add padding to prevent cut-off and make textarea taller visually
    lv_obj_set_style_pad_top(lvgl_deviceIPLastInput, 6, 0);
    lv_obj_set_style_pad_bottom(lvgl_deviceIPLastInput, 6, 0);
    lv_obj_set_style_pad_left(lvgl_deviceIPLastInput, 4, 0);
    lv_obj_set_style_pad_right(lvgl_deviceIPLastInput, 4, 0);
    // Add event handlers for keyboard show/hide
    lv_obj_add_event_cb(lvgl_deviceIPLastInput, textareaFocusedEventHandler, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(lvgl_deviceIPLastInput, textareaDefocusedEventHandler, LV_EVENT_DEFOCUSED, nullptr);
    
    // Connect button - normal size
    lvgl_deviceConnectBtn = lv_btn_create(ipInputRow);
    lv_obj_set_size(lvgl_deviceConnectBtn, LV_PCT(20), 40);  // Normal height 40px
    lv_obj_set_style_bg_color(lvgl_deviceConnectBtn, lv_color_hex(0x4CAF50), 0);  // Green
    lv_obj_add_event_cb(lvgl_deviceConnectBtn, deviceConnectBtnEventHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* connectBtnLabel = lv_label_create(lvgl_deviceConnectBtn);
    lv_label_set_text(connectBtnLabel, "Connect");  // Full text
    lv_obj_center(connectBtnLabel);
    
    // Right side: Previously connected devices section (50% width) - flex column layout
    lv_obj_t* prevDevicesSection = lv_obj_create(connectSection);
    lv_obj_set_width(prevDevicesSection, LV_PCT(48));
    lv_obj_set_height(prevDevicesSection, LV_SIZE_CONTENT);  // Auto-size
    lv_obj_set_style_bg_opa(prevDevicesSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(prevDevicesSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(prevDevicesSection, 0, 0);
    lv_obj_set_flex_flow(prevDevicesSection, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(prevDevicesSection, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(prevDevicesSection, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(prevDevicesSection, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* prevLabel = lv_label_create(prevDevicesSection);
    lv_label_set_text(prevLabel, "Previous:");
    lv_obj_set_width(prevLabel, LV_PCT(100));
    
    // Previous devices row container - flex row layout
    lv_obj_t* prevDevicesRow = lv_obj_create(prevDevicesSection);
    lv_obj_set_width(prevDevicesRow, LV_PCT(100));
    lv_obj_set_height(prevDevicesRow, 40);  // Normal height for dropdown/button
    lv_obj_set_style_bg_opa(prevDevicesRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(prevDevicesRow, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(prevDevicesRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(prevDevicesRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_layout(prevDevicesRow, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(prevDevicesRow, LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_deviceDropdown = lv_dropdown_create(prevDevicesRow);
    lv_obj_set_size(lvgl_deviceDropdown, LV_PCT(65), 40);  // Normal height 40px
    refreshDeviceDropdown();  // Populate from previouslyConnectedDevices
    
    lvgl_deviceDropdownConnectBtn = lv_btn_create(prevDevicesRow);
    lv_obj_set_size(lvgl_deviceDropdownConnectBtn, LV_PCT(30), 40);  // Normal height 40px
    lv_obj_set_style_bg_color(lvgl_deviceDropdownConnectBtn, lv_color_hex(0x4CAF50), 0);
    lv_obj_add_event_cb(lvgl_deviceDropdownConnectBtn, deviceDropdownConnectBtnEventHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* dropdownConnectLabel = lv_label_create(lvgl_deviceDropdownConnectBtn);
    lv_label_set_text(dropdownConnectLabel, "Connect");  // Full text
    lv_obj_center(dropdownConnectLabel);
    
    // Device list (scrollable container) - grid layout with 2 columns
    lvgl_deviceList = lv_obj_create(lvgl_devicesScreen);
    lv_obj_set_width(lvgl_deviceList, LV_PCT(100));
    lv_obj_set_height(lvgl_deviceList, LV_PCT(100));  // Take remaining space (flex grow)
    lv_obj_set_flex_grow(lvgl_deviceList, 1);  // Grow to fill available space
    lv_obj_set_style_bg_opa(lvgl_deviceList, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(lvgl_deviceList, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(lvgl_deviceList, 8, 0);  // Compact padding
    lv_obj_set_style_pad_column(lvgl_deviceList, 8, 0);  // Column spacing for grid
    lv_obj_set_style_pad_row(lvgl_deviceList, 8, 0);  // Row spacing for grid
    // Use flex with wrap to create a grid (2 columns)
    lv_obj_set_flex_flow(lvgl_deviceList, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(lvgl_deviceList, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(lvgl_deviceList, LV_LAYOUT_FLEX);
    lv_obj_set_scroll_dir(lvgl_deviceList, LV_DIR_VER);
    lv_obj_clear_flag(lvgl_deviceList, LV_OBJ_FLAG_SCROLL_ELASTIC);  // No elastic scrolling
    
    // Status label at bottom (fixed height)
    lvgl_deviceStatusLabel = lv_label_create(lvgl_devicesScreen);
    lv_obj_set_width(lvgl_deviceStatusLabel, LV_PCT(100));
    lv_obj_set_height(lvgl_deviceStatusLabel, 30);  // Fixed height
    lv_obj_set_style_text_align(lvgl_deviceStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lvgl_deviceStatusLabel, "No devices connected");
    
    // Create virtual keyboard (initially hidden) - numbers and dot only
    lvgl_keyboard = lv_keyboard_create(lvgl_devicesScreen);
    lv_obj_set_size(lvgl_keyboard, LV_PCT(100), LV_PCT(40));  // 40% of screen height
    lv_obj_align(lvgl_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    // Keyboard will be attached to focused textarea dynamically
    // Set keyboard to number mode (numbers and dot)
    lv_keyboard_set_mode(lvgl_keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_flag(lvgl_keyboard, LV_OBJ_FLAG_HIDDEN);  // Hide by default
    
    LOG_DEBUG_COMPONENT("LVGL", "Device management screen created");
}

static void deviceConnectBtnEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (lvgl_deviceIPPrefixInput == nullptr || lvgl_deviceIPLastInput == nullptr) {
            LOG_ERROR_COMPONENT("LVGL", "IP inputs not found");
            return;
        }
        
        // Get prefix and last octet
        const char* prefixText = lv_textarea_get_text(lvgl_deviceIPPrefixInput);
        const char* lastText = lv_textarea_get_text(lvgl_deviceIPLastInput);
        
        if (prefixText == nullptr || strlen(prefixText) == 0) {
            LOG_DEBUG_COMPONENT("LVGL", "No IP prefix entered");
            return;
        }
        
        if (lastText == nullptr || strlen(lastText) == 0) {
            LOG_DEBUG_COMPONENT("LVGL", "No last octet entered");
            return;
        }
        
        // Combine prefix and last octet
        String ipAddress = String(prefixText) + "." + String(lastText);
        LOG_INFOF_COMPONENT("LVGL", "Attempting to connect to device: %s", ipAddress.c_str());
        
        // Attempt connection
        if (DeviceManager::getInstance().connectDevice(ipAddress)) {
            LOG_INFOF_COMPONENT("LVGL", "Successfully initiated connection to %s", ipAddress.c_str());
            // Save to persistent storage
            addPreviouslyConnectedDevice(ipAddress);
            // Clear last octet input (keep prefix for next connection)
            lv_textarea_set_text(lvgl_deviceIPLastInput, "");
            // Update device list and dropdown
            updateDeviceList();
            refreshDeviceDropdown();
            updateDropdownConnectButtonState();
        } else {
            LOG_ERRORF_COMPONENT("LVGL", "Failed to connect to %s", ipAddress.c_str());
            // Could show error message here
        }
    }
}

static void deviceScreenBackBtnEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        LOG_DEBUG_COMPONENT("LVGL", "Back button clicked - returning to main screen");
        // Hide keyboard if visible
        if (lvgl_keyboard != nullptr) {
            lv_obj_add_flag(lvgl_keyboard, LV_OBJ_FLAG_HIDDEN);
        }
        hideDeviceManagement();
    }
}

static void textareaFocusedEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_FOCUSED) {
        LOG_DEBUG_COMPONENT("LVGL", "Textarea focused - showing keyboard");
        lv_obj_t* target = lv_event_get_target(e);
        if (lvgl_keyboard != nullptr && target != nullptr) {
            lv_keyboard_set_textarea(lvgl_keyboard, target);
            lv_obj_clear_flag(lvgl_keyboard, LV_OBJ_FLAG_HIDDEN);
            // Adjust device list height to make room for keyboard
            if (lvgl_deviceList != nullptr && lvgl_devicesScreen != nullptr) {
                lv_coord_t screenHeight = lv_obj_get_height(lvgl_devicesScreen);
                lv_coord_t keyboardHeight = lv_obj_get_height(lvgl_keyboard);
                lv_obj_set_height(lvgl_deviceList, screenHeight - 260 - keyboardHeight);
            }
        }
    }
}

static void textareaDefocusedEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_DEFOCUSED) {
        LOG_DEBUG_COMPONENT("LVGL", "Textarea defocused - hiding keyboard");
        if (lvgl_keyboard != nullptr) {
            lv_obj_add_flag(lvgl_keyboard, LV_OBJ_FLAG_HIDDEN);
            // Restore device list height
            if (lvgl_deviceList != nullptr && lvgl_devicesScreen != nullptr) {
                lv_coord_t screenHeight = lv_obj_get_height(lvgl_devicesScreen);
                lv_obj_set_height(lvgl_deviceList, screenHeight - 260);
            }
        }
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
            LOG_DEBUGF_COMPONENT("LVGL", "Setting brightness for %s to %d", ip.c_str(), brightness);
            DeviceManager::getInstance().sendBrightnessToDevice(ip, brightness);
        } else {
            LOG_ERROR_COMPONENT("LVGL", "Could not find IP address for brightness slider");
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
            LOG_INFOF_COMPONENT("LVGL", "Disconnecting device: %s", ip.c_str());
            DeviceManager::getInstance().disconnectDevice(ip);
            // Update device list
            updateDeviceList();
        } else {
            LOG_ERROR_COMPONENT("LVGL", "Could not find IP address for disconnect button");
        }
    }
}

static void deviceNextEffectBtnEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* btn = lv_event_get_target(e);
        // Get IP address by finding the container in our map
        String ip = getIPFromContainer(btn);
        if (ip.length() > 0) {
            LOG_DEBUGF_COMPONENT("LVGL", "Triggering next effect for device: %s", ip.c_str());
            // Send next_effect command to this specific device
            String command = "{\"t\":\"next_effect\"}";
            if (DeviceManager::getInstance().sendCommandToDevice(ip, command)) {
                LOG_DEBUGF_COMPONENT("LVGL", "Next effect command sent to %s", ip.c_str());
            } else {
                LOG_WARNF_COMPONENT("LVGL", "Failed to send next effect command to %s", ip.c_str());
            }
        } else {
            LOG_ERROR_COMPONENT("LVGL", "Could not find IP address for next effect button");
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
    
        LOG_DEBUGF_COMPONENT("LVGL", "Creating UI item for device: %s (%s)", ipAddress.c_str(), displayName.c_str());
    
    // Create device container - auto-size with flexbox, very compact, fixed width for grid
    lv_obj_t* deviceContainer = lv_obj_create(lvgl_deviceList);
    lv_obj_set_width(deviceContainer, LV_PCT(48));  // ~50% width for 2-column grid (accounting for padding)
    lv_obj_set_height(deviceContainer, LV_SIZE_CONTENT);  // Auto-size based on content
    lv_obj_set_style_bg_color(deviceContainer, lv_color_hex(0xF5F5F5), 0);  // Slightly lighter
    lv_obj_set_style_border_width(deviceContainer, 2, 0);
    lv_obj_set_style_border_color(deviceContainer, lv_color_hex(0xCCCCCC), 0);  // Lighter border
    lv_obj_set_style_radius(deviceContainer, 8, 0);  // Slightly more rounded
    lv_obj_set_style_pad_all(deviceContainer, 8, 0);  // Very compact padding
    lv_obj_set_style_pad_row(deviceContainer, 4, 0);  // Minimal vertical spacing
    lv_obj_set_flex_flow(deviceContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(deviceContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(deviceContainer, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(deviceContainer, LV_OBJ_FLAG_SCROLLABLE);  // Container itself is not scrollable
    
    // Store container in map
    deviceUIContainers[ipAddress] = deviceContainer;
    
    // Device info section (header with status, name/IP, and disconnect button)
    lv_obj_t* infoSection = lv_obj_create(deviceContainer);
    lv_obj_set_width(infoSection, LV_PCT(100));
    lv_obj_set_height(infoSection, LV_SIZE_CONTENT);  // Auto-size
    lv_obj_set_style_bg_opa(infoSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(infoSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(infoSection, 0, 0);
    lv_obj_set_flex_flow(infoSection, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(infoSection, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_layout(infoSection, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(infoSection, LV_OBJ_FLAG_SCROLLABLE);
    
    // Left side: Status indicator + Device name/IP
    lv_obj_t* deviceInfoLeft = lv_obj_create(infoSection);
    lv_obj_set_width(deviceInfoLeft, LV_SIZE_CONTENT);
    lv_obj_set_height(deviceInfoLeft, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(deviceInfoLeft, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(deviceInfoLeft, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(deviceInfoLeft, 0, 0);
    lv_obj_set_flex_flow(deviceInfoLeft, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(deviceInfoLeft, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_layout(deviceInfoLeft, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(deviceInfoLeft, LV_OBJ_FLAG_SCROLLABLE);
    
    // Status indicator (colored circle) - compact
    lv_obj_t* statusIndicator = lv_obj_create(deviceInfoLeft);
    lv_obj_set_size(statusIndicator, 16, 16);  // Smaller, more compact
    lv_obj_set_style_bg_color(statusIndicator, isConnected ? lv_color_hex(0x4CAF50) : lv_color_hex(0xF44336), 0);
    lv_obj_set_style_bg_opa(statusIndicator, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(statusIndicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_opa(statusIndicator, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_right(statusIndicator, 8, 0);  // Compact spacing
    
    // Device name/IP label - separate lines for better readability
    lv_obj_t* deviceLabel = lv_label_create(deviceInfoLeft);
    char labelText[128];
    snprintf(labelText, sizeof(labelText), "%s\n%s", displayName.c_str(), ipAddress.c_str());
    lv_label_set_text(deviceLabel, labelText);
    lv_obj_set_style_text_align(deviceLabel, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_clear_flag(deviceLabel, LV_OBJ_FLAG_SCROLLABLE);
    
    // Disconnect button - small power icon button on the right
    lv_obj_t* disconnectBtn = lv_btn_create(infoSection);
    lv_obj_set_size(disconnectBtn, 32, 32);  // Small square button
    lv_obj_set_style_bg_color(disconnectBtn, lv_color_hex(0xF44336), 0);  // Red
    lv_obj_set_style_radius(disconnectBtn, LV_RADIUS_CIRCLE, 0);  // Circular
    lv_obj_set_style_border_width(disconnectBtn, 0, 0);
    lv_obj_set_style_pad_all(disconnectBtn, 0, 0);  // No padding
    // No need to store IP - we'll look it up from the container hierarchy
    lv_obj_add_event_cb(disconnectBtn, deviceDisconnectBtnEventHandler, LV_EVENT_CLICKED, nullptr);
    lv_obj_clear_flag(disconnectBtn, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* disconnectBtnLabel = lv_label_create(disconnectBtn);
    lv_label_set_text(disconnectBtnLabel, LV_SYMBOL_POWER);  // Power icon
    lv_obj_set_style_text_color(disconnectBtnLabel, lv_color_white(), 0);
    lv_obj_center(disconnectBtnLabel);
    
    // Brightness control section - very compact, single row: label + slider + button
    lv_obj_t* brightnessSection = lv_obj_create(deviceContainer);
    lv_obj_set_width(brightnessSection, LV_PCT(100));
    lv_obj_set_height(brightnessSection, LV_SIZE_CONTENT);  // Auto-size based on content
    lv_obj_set_style_bg_opa(brightnessSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(brightnessSection, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(brightnessSection, 0, 0);
    lv_obj_set_flex_flow(brightnessSection, LV_FLEX_FLOW_ROW);  // Single row: label + slider + button
    lv_obj_set_flex_align(brightnessSection, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_layout(brightnessSection, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(brightnessSection, LV_OBJ_FLAG_SCROLLABLE);
    
    // Brightness label with value - compact, on the left
    lv_obj_t* brightnessLabel = lv_label_create(brightnessSection);
    lv_label_set_text(brightnessLabel, "Brightness: 50%");
    lv_obj_set_style_text_align(brightnessLabel, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_clear_flag(brightnessLabel, LV_OBJ_FLAG_SCROLLABLE);
    
    // Brightness slider - takes remaining space
    lv_obj_t* brightnessSlider = lv_slider_create(brightnessSection);
    lv_obj_set_flex_grow(brightnessSlider, 1);  // Take all available space
    lv_obj_set_height(brightnessSlider, 20);  // Very compact height
    lv_obj_set_style_pad_left(brightnessSlider, 8, 0);  // Small gap from label
    lv_obj_set_style_pad_right(brightnessSlider, 8, 0);  // Small gap to button
    lv_slider_set_range(brightnessSlider, 0, 100);
    lv_slider_set_value(brightnessSlider, 50, LV_ANIM_OFF);
    // No need to store IP - we'll look it up from the container hierarchy
    lv_obj_add_event_cb(brightnessSlider, deviceBrightnessSliderEventHandler, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_clear_flag(brightnessSlider, LV_OBJ_FLAG_SCROLLABLE);
    
    // Next effect button - all the way to the right, compact
    lv_obj_t* nextEffectBtn = lv_btn_create(brightnessSection);
    lv_obj_set_size(nextEffectBtn, 28, 28);  // Very compact square button
    lv_obj_set_style_bg_color(nextEffectBtn, lv_color_hex(0x2196F3), 0);  // Blue
    lv_obj_set_style_radius(nextEffectBtn, 5, 0);
    lv_obj_set_style_border_width(nextEffectBtn, 1, 0);
    lv_obj_set_style_border_color(nextEffectBtn, lv_color_black(), 0);
    lv_obj_set_style_pad_all(nextEffectBtn, 0, 0);  // No padding to maximize icon size
    // No need to store IP - we'll look it up from the container hierarchy
    lv_obj_add_event_cb(nextEffectBtn, deviceNextEffectBtnEventHandler, LV_EVENT_CLICKED, nullptr);
    lv_obj_clear_flag(nextEffectBtn, LV_OBJ_FLAG_SCROLLABLE);
    
    // Button label with next icon
    lv_obj_t* nextEffectBtnLabel = lv_label_create(nextEffectBtn);
    lv_label_set_text(nextEffectBtnLabel, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(nextEffectBtnLabel, lv_color_white(), 0);
    lv_obj_center(nextEffectBtnLabel);
}

static void removeDeviceListItem(const String& ipAddress) {
    auto it = deviceUIContainers.find(ipAddress);
    if (it != deviceUIContainers.end()) {
        LOG_DEBUGF_COMPONENT("LVGL", "Removing UI item for device: %s", ipAddress.c_str());
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
        LOG_ERRORF_COMPONENT("LVGL", "Failed to parse device list JSON: %s", error.c_str());
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
    // Collect IPs to remove first to avoid iterator invalidation
    std::vector<String> toRemove;
    for (const auto& pair : deviceUIContainers) {
        if (currentDevices.find(pair.first) == currentDevices.end()) {
            toRemove.push_back(pair.first);
        }
    }
    // Remove each device (removeDeviceListItem already erases from map)
    for (const String& ip : toRemove) {
        removeDeviceListItem(ip);
    }
    
    LOG_DEBUGF_COMPONENT("LVGL", "Device list updated: %d devices", deviceCount);
    
    // Update dropdown connect button state
    updateDropdownConnectButtonState();
}

// Device storage functions
static bool loadPreviouslyConnectedDevices() {
    extern SDCardController* g_sdCardController;
    
    if (!g_sdCardController || !g_sdCardController->isAvailable()) {
        LOG_DEBUG_COMPONENT("DeviceStorage", "SD card not available");
        return false;
    }
    
    String jsonString = g_sdCardController->readFile("/data/connected_devices.json");
    if (jsonString.length() == 0) {
        LOG_DEBUG_COMPONENT("DeviceStorage", "File not found or empty");
        return false;
    }
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        LOG_ERRORF_COMPONENT("DeviceStorage", "Failed to parse JSON: %s", error.c_str());
        return false;
    }
    
    previouslyConnectedDevices.clear();
    if (doc.containsKey("previouslyConnectedDevices")) {
        JsonArray devices = doc["previouslyConnectedDevices"];
        for (JsonVariant device : devices) {
            String ip = device.as<String>();
            if (ip.length() > 0) {
                previouslyConnectedDevices.push_back(ip);
            }
        }
    }
    
    LOG_INFOF_COMPONENT("DeviceStorage", "Loaded %d previously connected devices", previouslyConnectedDevices.size());
    return true;
}

static bool savePreviouslyConnectedDevices() {
    extern SDCardController* g_sdCardController;
    
    if (!g_sdCardController || !g_sdCardController->isAvailable()) {
        return false;
    }
    
    StaticJsonDocument<1024> doc;
    JsonArray devices = doc.createNestedArray("previouslyConnectedDevices");
    
    for (const String& ip : previouslyConnectedDevices) {
        devices.add(ip);
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    bool success = g_sdCardController->writeFile("/data/connected_devices.json", jsonString.c_str());
    if (success) {
        LOG_INFOF_COMPONENT("DeviceStorage", "Saved %d devices to storage", previouslyConnectedDevices.size());
    } else {
        LOG_ERROR_COMPONENT("DeviceStorage", "Failed to save devices");
    }
    return success;
}

static void addPreviouslyConnectedDevice(const String& ipAddress) {
    // Check if already in list
    for (const String& existing : previouslyConnectedDevices) {
        if (existing == ipAddress) {
            return;  // Already exists
        }
    }
    
    previouslyConnectedDevices.push_back(ipAddress);
    savePreviouslyConnectedDevices();
    LOG_INFOF_COMPONENT("DeviceStorage", "Added device: %s", ipAddress.c_str());
}

// Dropdown UI functions
static void refreshDeviceDropdown() {
    if (lvgl_deviceDropdown == nullptr) return;
    
    String options = "";
    for (size_t i = 0; i < previouslyConnectedDevices.size(); i++) {
        if (i > 0) options += "\n";
        options += previouslyConnectedDevices[i];
    }
    
    if (options.length() == 0) {
        options = "No previous devices";
    }
    
    lv_dropdown_set_options(lvgl_deviceDropdown, options.c_str());
    if (previouslyConnectedDevices.size() > 0) {
        lv_dropdown_set_selected(lvgl_deviceDropdown, 0);
    }
    updateDropdownConnectButtonState();
}

static void updateDropdownConnectButtonState() {
    if (lvgl_deviceDropdownConnectBtn == nullptr || lvgl_deviceDropdown == nullptr) {
        return;
    }
    
    if (previouslyConnectedDevices.size() == 0) {
        lv_obj_add_state(lvgl_deviceDropdownConnectBtn, LV_STATE_DISABLED);
        return;
    }
    
    char selected[64];
    lv_dropdown_get_selected_str(lvgl_deviceDropdown, selected, sizeof(selected));
    String ip = String(selected);
    
    bool isConnected = DeviceManager::getInstance().isDeviceConnected(ip);
    
    if (isConnected) {
        lv_obj_add_state(lvgl_deviceDropdownConnectBtn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(lvgl_deviceDropdownConnectBtn, lv_color_hex(0x808080), 0);
    } else {
        lv_obj_clear_state(lvgl_deviceDropdownConnectBtn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(lvgl_deviceDropdownConnectBtn, lv_color_hex(0x4CAF50), 0);
    }
}

static void deviceDropdownConnectBtnEventHandler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    if (lvgl_deviceDropdown == nullptr) return;
    
    char selected[64];
    lv_dropdown_get_selected_str(lvgl_deviceDropdown, selected, sizeof(selected));
    String ipAddress = String(selected);
    
    if (ipAddress.length() == 0 || ipAddress == "No previous devices") {
        return;
    }
    
    LOG_INFOF_COMPONENT("LVGL", "Connecting to previous device: %s", ipAddress.c_str());
    
    if (DeviceManager::getInstance().connectDevice(ipAddress)) {
        addPreviouslyConnectedDevice(ipAddress);  // Ensure it's saved
        updateDeviceList();
        refreshDeviceDropdown();
        updateDropdownConnectButtonState();
    } else {
        LOG_ERRORF_COMPONENT("LVGL", "Failed to connect to previous device: %s", ipAddress.c_str());
    }
}

// Public function to load devices (called from main.cpp)
void loadPreviouslyConnectedDevicesFromStorage() {
    loadPreviouslyConnectedDevices();
}

#endif // PLATFORM_CROW_PANEL

