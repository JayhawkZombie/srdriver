#pragma once

#if PLATFORM_CROW_PANEL
#include <lvgl.h>

/**
 * Device Management UI
 * 
 * Handles device management screen including:
 * - IP address input
 * - Connect/disconnect buttons
 * - Device list with status
 * - Per-device brightness controls
 */

// Device management screen objects (extern - defined in lvgl_devices.cpp)
extern lv_obj_t* lvgl_devicesScreen;
extern lv_obj_t* lvgl_deviceIPPrefixInput;  // IP prefix input (e.g., "192.168.1")
extern lv_obj_t* lvgl_deviceIPLastInput;    // Last octet input (e.g., "163")
extern lv_obj_t* lvgl_deviceConnectBtn;
extern lv_obj_t* lvgl_deviceList;  // Scrollable container for device list
extern lv_obj_t* lvgl_deviceStatusLabel;  // Status text at bottom
extern lv_obj_t* lvgl_keyboard;  // Virtual keyboard for text input

/**
 * Show device management screen
 */
void showDeviceManagement();

/**
 * Hide device management screen (return to main screen)
 */
void hideDeviceManagement();

/**
 * Update device list UI (refresh all devices)
 */
void updateDeviceList();

/**
 * Check if device management screen is currently shown
 */
bool isDeviceManagementShown();

#endif // PLATFORM_CROW_PANEL

