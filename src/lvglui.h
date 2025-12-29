#pragma once

#if PLATFORM_CROW_PANEL
#include <lvgl.h>

/**
 * LVGL UI Management
 * 
 * Handles creation and updates of LVGL UI elements including:
 * - Button grid layout
 * - System button (shows uptime)
 * - WiFi button (opens WiFi info message box)
 * - WiFi information message box
 */

// UI object pointers (extern - defined in lvglui.cpp)
extern lv_obj_t* lvgl_screen;
extern lv_obj_t* lvgl_buttonGrid;
extern lv_obj_t* lvgl_systemButton;
extern lv_obj_t* lvgl_systemButtonLabel;
extern lv_obj_t* lvgl_wifiButton;
extern lv_obj_t* lvgl_wifiButtonLabel;

// WiFi message box objects
extern lv_obj_t* lvgl_wifiMsgBox;
extern lv_obj_t* lvgl_wifiMsgBoxBackdrop;
extern lv_obj_t* lvgl_wifiMsgBoxContent;
extern lv_obj_t* lvgl_wifiMsgBoxCloseBtn;
extern lv_obj_t* lvgl_wifiMsgBoxText;

// WiFi message box state
extern uint32_t lvgl_wifiMsgBoxUpdateInterval;
extern uint32_t lvgl_lastWifiMsgBoxUpdate;

/**
 * Create the LVGL UI with button grid
 */
void createLVGLUI();

/**
 * Update system button with current uptime
 */
void updateSystemButton();

/**
 * Update WiFi message box content (if open)
 */
void updateWiFiMessageBox();

/**
 * Show WiFi information message box
 */
void ShowWiFiInformation();

/**
 * Close WiFi message box
 */
void closeWiFiMessageBox();

#endif // PLATFORM_CROW_PANEL

