#pragma once

#if PLATFORM_CROW_PANEL
#include <lvgl.h>

/**
 * System Menu UI
 * 
 * Handles the system settings menu with tabs for:
 * - Display settings
 * - Network settings
 * - About information
 */

// System menu objects (extern - defined in lvgl_system_menu.cpp)
extern lv_obj_t* lvgl_systemMenu;

/**
 * Create and show the system menu
 */
void showSystemMenu();

/**
 * Hide the system menu (return to main screen)
 */
void hideSystemMenu();

/**
 * Check if system menu is currently shown
 */
bool isSystemMenuShown();

#endif // PLATFORM_CROW_PANEL

