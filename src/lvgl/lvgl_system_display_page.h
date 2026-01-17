#pragma once

#if PLATFORM_CROW_PANEL
#include <lvgl.h>

/**
 * System Display Settings Page
 * 
 * Creates the Display settings page for the system menu
 */

/**
 * Create the Display settings page
 * @param menu The parent menu object
 * @return The created page object
 */
lv_obj_t* createSystemDisplayPage(lv_obj_t* menu);

#endif // PLATFORM_CROW_PANEL

