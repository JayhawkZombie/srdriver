#include "lvgl_system_networks_page.h"
#include "freertos/LogManager.h"
#include "freertos/TaskManager.h"
#include "freertos/WiFiManager.h"

#if PLATFORM_CROW_PANEL

// Store references to UI elements for updates
static lv_obj_t* currentSSIDItem = nullptr;
static lv_obj_t* currentSignalItem = nullptr;
static lv_obj_t* currentIPItem = nullptr;
static lv_obj_t* currentStatusItem = nullptr;
static lv_obj_t* knownNetworksContainer = nullptr;

// Forward declarations
static void updateCurrentNetworkInfo(lv_obj_t* page);
static void updateKnownNetworksList(lv_obj_t* page);

// Helper function to create a text-only menu item (for displaying info)
static lv_obj_t* createTextMenuItem(lv_obj_t* parent, const char* icon, const char* label, const char* value) {
    lv_obj_t* obj = lv_menu_cont_create(parent);
    
    if (icon) {
        lv_obj_t* img = lv_label_create(obj);
        lv_label_set_text(img, icon);
    }
    
    if (label) {
        lv_obj_t* labelObj = lv_label_create(obj);
        lv_label_set_text(labelObj, label);
        lv_label_set_long_mode(labelObj, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(labelObj, 1);
    }
    
    if (value) {
        lv_obj_t* valueObj = lv_label_create(obj);
        lv_label_set_text(valueObj, value);
        lv_label_set_long_mode(valueObj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    }
    
    return obj;
}

// Helper function to create a known network menu item (with SSID and action buttons placeholder)
static lv_obj_t* createKnownNetworkItem(lv_obj_t* parent, const char* ssid) {
    lv_obj_t* obj = lv_menu_cont_create(parent);
    
    // WiFi icon
    lv_obj_t* icon = lv_label_create(obj);
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    
    // SSID label
    lv_obj_t* ssidLabel = lv_label_create(obj);
    lv_label_set_text(ssidLabel, ssid);
    lv_label_set_long_mode(ssidLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(ssidLabel, 1);
    
    // TODO: Add connect button here
    // TODO: Add remove button here
    
    return obj;
}

static void updateCurrentNetworkInfo(lv_obj_t* page) {
    // This will be called to update the current network display
    // For now, just a placeholder
    LOG_DEBUG_COMPONENT("LVGL", "Updating current network info");
}

static void updateKnownNetworksList(lv_obj_t* page) {
    // This will be called to update the known networks list
    // For now, just a placeholder
    LOG_DEBUG_COMPONENT("LVGL", "Updating known networks list");
}

lv_obj_t* createSystemNetworksPage(lv_obj_t* menu) {
    LOG_DEBUG_COMPONENT("LVGL", "Creating Networks settings page...");
    
    // Create page
    lv_obj_t* page = lv_menu_page_create(menu, nullptr);
    lv_obj_t* header = lv_menu_get_main_header(menu);
    lv_obj_set_style_pad_hor(page, lv_obj_get_style_pad_left(header, LV_PART_MAIN), 0);
    
    // Add separator
    lv_menu_separator_create(page);
    
    // ===== Current Network Section =====
    lv_obj_t* currentNetworkSection = lv_menu_section_create(page);
    
    // Section title (using a text item as label)
    createTextMenuItem(currentNetworkSection, nullptr, "Current Network", nullptr);
    
    // Network SSID (store reference for updates)
    currentSSIDItem = createTextMenuItem(currentNetworkSection, LV_SYMBOL_WIFI, "SSID:", "(not connected)");
    
    // Signal strength (RSSI) (store reference for updates)
    currentSignalItem = createTextMenuItem(currentNetworkSection, nullptr, "Signal:", "N/A");
    
    // IP Address (store reference for updates)
    currentIPItem = createTextMenuItem(currentNetworkSection, nullptr, "IP Address:", "(not connected)");
    
    // Connection status (store reference for updates)
    currentStatusItem = createTextMenuItem(currentNetworkSection, nullptr, "Status:", "disconnected");
    
    // TODO: Add disconnect button if connected
    
    // ===== Known Networks Section =====
    // Add separator before known networks
    lv_menu_separator_create(page);
    
    // Section title
    lv_obj_t* knownNetworksSection = lv_menu_section_create(page);
    createTextMenuItem(knownNetworksSection, nullptr, "Known Networks", nullptr);
    
    // Store reference to container for updates
    knownNetworksContainer = knownNetworksSection;
    
    // TODO: Populate with known networks from WiFiManager
    // For now, just a placeholder item
    createKnownNetworkItem(knownNetworksSection, "(No saved networks)");
    
    // Initial update of network info
    updateCurrentNetworkInfo(page);
    updateKnownNetworksList(page);
    
    LOG_DEBUG_COMPONENT("LVGL", "Networks settings page created");
    return page;
}

void updateSystemNetworksPage(lv_obj_t* page) {
    if (page == nullptr) {
        return;
    }
    updateCurrentNetworkInfo(page);
    updateKnownNetworksList(page);
}

#endif // PLATFORM_CROW_PANEL

