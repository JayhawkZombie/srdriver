#pragma once

#if PLATFORM_CROW_PANEL
#include <lvgl.h>

/**
 * System About Page
 * 
 * Creates the About information page for the system menu
 */

/**
 * Create the About page
 * @param menu The parent menu object
 * @return The created page object
 */
lv_obj_t* createSystemAboutPage(lv_obj_t* menu);

#endif // PLATFORM_CROW_PANEL

