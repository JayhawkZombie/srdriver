#include "lvgl_system_display_page.h"
#include "freertos/LogManager.h"

#if PLATFORM_CROW_PANEL

// Forward declaration
static void displayBrightnessSliderEventHandler(lv_event_t* e);

// Helper function to create a menu item with slider (similar to LVGL example)
static lv_obj_t* createSliderMenuItem(lv_obj_t* parent, const char* icon, const char* txt, int32_t min, int32_t max, int32_t val) {
    // Create menu container
    lv_obj_t* obj = lv_menu_cont_create(parent);
    
    // Add icon if provided
    if (icon) {
        lv_obj_t* img = lv_label_create(obj);
        lv_label_set_text(img, icon);
    }
    
    // Add text label if provided
    if (txt) {
        lv_obj_t* label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }
    
    // Create slider
    lv_obj_t* slider = lv_slider_create(obj);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);
    
    if (icon == nullptr) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }
    
    return obj;
}

static void displayBrightnessSliderEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* slider = lv_event_get_target(e);
        int32_t brightness = lv_slider_get_value(slider);
        
        // Set display brightness (0-255 PWM value)
        ledcWrite(1, brightness);
        
        LOG_DEBUGF_COMPONENT("LVGL", "Display brightness set to %d", brightness);
    }
}

lv_obj_t* createSystemDisplayPage(lv_obj_t* menu) {
    LOG_DEBUG_COMPONENT("LVGL", "Creating Display settings page...");
    
    // Create page
    lv_obj_t* page = lv_menu_page_create(menu, nullptr);
    lv_obj_t* header = lv_menu_get_main_header(menu);
    lv_obj_set_style_pad_hor(page, lv_obj_get_style_pad_left(header, LV_PART_MAIN), 0);
    
    // Add separator
    lv_menu_separator_create(page);
    
    // Create section
    lv_obj_t* section = lv_menu_section_create(page);
    
    // Create brightness slider (0-255 range to match PWM)
    // Default to 50 (matching main.cpp initialization)
    lv_obj_t* brightnessItem = createSliderMenuItem(section, LV_SYMBOL_SETTINGS, "Brightness", 0, 255, 50);
    lv_obj_t* brightnessSlider = lv_obj_get_child(brightnessItem, lv_obj_get_child_cnt(brightnessItem) - 1);
    lv_obj_add_event_cb(brightnessSlider, displayBrightnessSliderEventHandler, LV_EVENT_VALUE_CHANGED, nullptr);
    
    LOG_DEBUG_COMPONENT("LVGL", "Display settings page created");
    return page;
}

#endif // PLATFORM_CROW_PANEL

