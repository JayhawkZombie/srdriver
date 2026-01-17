#pragma once

#if PLATFORM_CROW_PANEL
#include <lvgl.h>

/**
 * System Networks Settings Page
 * 
 * Creates the Networks settings page for the system menu with:
 * - Current network information (SSID, signal strength, IP address, status)
 * - List of known/saved networks
 */

/**
 * Create the Networks settings page
 * @param menu The parent menu object
 * @return The created page object
 */
lv_obj_t* createSystemNetworksPage(lv_obj_t* menu);

/**
 * Update the current network information display
 * @param page The networks page object
 */
void updateSystemNetworksPage(lv_obj_t* page);

#endif // PLATFORM_CROW_PANEL

