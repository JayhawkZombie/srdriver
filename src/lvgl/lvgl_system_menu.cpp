#include "lvgl_system_menu.h"
#include "lvgl_system_display_page.h"
#include "lvgl_system_networks_page.h"
#include "lvgl_system_about_page.h"
#include "lvglui.h"  // For lvgl_screen access
#include "freertos/LogManager.h"

#if PLATFORM_CROW_PANEL

// System menu object
lv_obj_t* lvgl_systemMenu = nullptr;

// Forward declarations
static void createSystemMenu();
static void systemMenuBackEventHandler(lv_event_t* e);

// Helper function to create a menu item (text only)
static lv_obj_t* createMenuItem(lv_obj_t* parent, const char* icon, const char* text) {
    lv_obj_t* item = lv_menu_cont_create(parent);
    
    if (icon) {
        lv_obj_t* img = lv_label_create(item);
        lv_label_set_text(img, icon);
    }
    
    if (text) {
        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, text);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }
    
    return item;
}

void showSystemMenu() {
    LOG_DEBUG_COMPONENT("LVGL", "Showing system menu...");
    
    // Create menu if it doesn't exist
    if (lvgl_systemMenu == nullptr) {
        createSystemMenu();
    }
    
    // Load the menu screen
    lv_scr_load(lvgl_systemMenu);
    
    LOG_DEBUG_COMPONENT("LVGL", "System menu shown");
}

void hideSystemMenu() {
    if (lvgl_systemMenu == nullptr) {
        return;
    }
    
    // Return to main screen
    if (lvgl_screen != nullptr) {
        lv_scr_load(lvgl_screen);
        LOG_DEBUG_COMPONENT("LVGL", "Returned to main screen from system menu");
    }
}

bool isSystemMenuShown() {
    return lvgl_systemMenu != nullptr && lv_scr_act() == lvgl_systemMenu;
}

static void systemMenuBackEventHandler(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_t* menu = (lv_obj_t*)lv_event_get_user_data(e);
    
    if (lv_menu_back_btn_is_root(menu, obj)) {
        LOG_DEBUG_COMPONENT("LVGL", "System menu root back button clicked");
        hideSystemMenu();
    }
}

static void createSystemMenu() {
    LOG_DEBUG_COMPONENT("LVGL", "Creating system menu...");
    
    // Create menu as a screen-like object
    lvgl_systemMenu = lv_menu_create(nullptr);
    if (!lvgl_systemMenu) {
        LOG_ERROR_COMPONENT("LVGL", "Failed to create system menu");
        return;
    }
    
    // Set menu size to fill screen
    lv_obj_set_size(lvgl_systemMenu, LV_PCT(100), LV_PCT(100));
    
    // Enable root back button mode (this creates the back button in the header)
    lv_menu_set_mode_root_back_btn(lvgl_systemMenu, LV_MENU_ROOT_BACK_BTN_ENABLED);
    
    // Add event handler to the menu itself (not individual items)
    lv_obj_add_event_cb(lvgl_systemMenu, systemMenuBackEventHandler, LV_EVENT_CLICKED, lvgl_systemMenu);
    
    // Get header for padding reference
    lv_obj_t* header = lv_menu_get_main_header(lvgl_systemMenu);
    
    // Create root page (sidebar)
    static char settingsTitle[] = "Settings";
    lv_obj_t* rootPage = lv_menu_page_create(lvgl_systemMenu, settingsTitle);
    lv_obj_set_style_pad_hor(rootPage, lv_obj_get_style_pad_left(header, LV_PART_MAIN), 0);
    
    // Create "Settings" section
    lv_obj_t* settingsSection = lv_menu_section_create(rootPage);
    
    // Display menu item
    lv_obj_t* displayItem = createMenuItem(settingsSection, LV_SYMBOL_SETTINGS, "Display");
    lv_obj_t* displayPage = createSystemDisplayPage(lvgl_systemMenu);
    lv_menu_set_load_page_event(lvgl_systemMenu, displayItem, displayPage);
    
    // Networks menu item
    lv_obj_t* networksItem = createMenuItem(settingsSection, LV_SYMBOL_WIFI, "Networks");
    lv_obj_t* networksPage = createSystemNetworksPage(lvgl_systemMenu);
    lv_menu_set_load_page_event(lvgl_systemMenu, networksItem, networksPage);
    
    // Create "Others" section
    lv_obj_t* othersLabel = lv_label_create(rootPage);
    lv_label_set_text(othersLabel, "Others");
    
    lv_obj_t* othersSection = lv_menu_section_create(rootPage);
    
    // About menu item
    lv_obj_t* aboutItem = createMenuItem(othersSection, nullptr, "About");
    lv_obj_t* aboutPage = createSystemAboutPage(lvgl_systemMenu);
    lv_menu_set_load_page_event(lvgl_systemMenu, aboutItem, aboutPage);
    
    // Set root page as sidebar
    lv_menu_set_sidebar_page(lvgl_systemMenu, rootPage);
    
    // Load the first page (Display) initially
    lv_menu_set_page(lvgl_systemMenu, displayPage);
    
    LOG_DEBUG_COMPONENT("LVGL", "System menu created");
}

#endif // PLATFORM_CROW_PANEL

