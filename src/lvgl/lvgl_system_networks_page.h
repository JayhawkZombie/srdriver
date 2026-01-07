#pragma once

#if PLATFORM_CROW_PANEL
#include <lvgl.h>

/**
 * System Networks Settings Page
 * 
 * Creates the Networks settings page for the system menu
 */

/**
 * Create the Networks settings page
 * @param menu The parent menu object
 * @return The created page object
 */
lv_obj_t* createSystemNetworksPage(lv_obj_t* menu);

#endif // PLATFORM_CROW_PANEL

