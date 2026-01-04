#include "lvglui.h"
#include "lvgl_devices.h"
#include "task_view.h"
#include "freertos/TaskManager.h"
#include "freertos/WiFiManager.h"
#include "freertos/SystemMonitorTask.h"
#include "PatternManager.h"

#if PLATFORM_CROW_PANEL

// UI object pointers
lv_obj_t* lvgl_screen = nullptr;
lv_obj_t* lvgl_buttonGrid = nullptr;
lv_obj_t* lvgl_systemButton = nullptr;
lv_obj_t* lvgl_systemButtonLabel = nullptr;
lv_obj_t* lvgl_wifiButton = nullptr;
lv_obj_t* lvgl_wifiButtonLabel = nullptr;

// WiFi message box objects
lv_obj_t* lvgl_wifiMsgBox = nullptr;
lv_obj_t* lvgl_wifiMsgBoxBackdrop = nullptr;
lv_obj_t* lvgl_wifiMsgBoxContent = nullptr;
lv_obj_t* lvgl_wifiMsgBoxCloseBtn = nullptr;
lv_obj_t* lvgl_wifiMsgBoxText = nullptr;

// WiFi message box state
uint32_t lvgl_wifiMsgBoxUpdateInterval = 1000;  // 1 second default
uint32_t lvgl_lastWifiMsgBoxUpdate = 0;

// Forward declarations
static void createButtonGrid();
static void createSystemButton();
static void createWiFiButton();
static void createDevicesButton();
static void createEffectsButton();
static void systemButtonEventHandler(lv_event_t* e);
static void wifiButtonEventHandler(lv_event_t* e);
static void devicesButtonEventHandler(lv_event_t* e);
static void effectsButtonEventHandler(lv_event_t* e);
static void msgboxCloseEventHandler(lv_event_t* e);
static void msgboxBackdropEventHandler(lv_event_t* e);

void createLVGLUI() {
    Serial.println("[LVGL] Creating UI...");
    
    // Create main screen
    lvgl_screen = lv_obj_create(nullptr);
    if (!lvgl_screen) {
        Serial.println("[LVGL] ERROR: Failed to create screen!");
        return;
    }
    
    lv_obj_set_style_bg_color(lvgl_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lvgl_screen, LV_OPA_COVER, 0);
    lv_scr_load(lvgl_screen);
    Serial.println("[LVGL] Screen created and loaded");
    
    // Create button grid
    createButtonGrid();
    
    // Force invalidate entire screen
    lv_obj_invalidate(lvgl_screen);
    Serial.println("[LVGL] UI creation complete");
}

static void createButtonGrid() {
    Serial.println("[LVGL] Creating button grid...");
    
    // Create flex container for 2x2 grid
    lvgl_buttonGrid = lv_obj_create(lvgl_screen);
    lv_obj_set_size(lvgl_buttonGrid, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(lvgl_buttonGrid, 0, 0);
    lv_obj_set_style_bg_opa(lvgl_buttonGrid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(lvgl_buttonGrid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(lvgl_buttonGrid, 10, 0);
    
    // Set flex layout: 2 columns, 2 rows
    lv_obj_set_layout(lvgl_buttonGrid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lvgl_buttonGrid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(lvgl_buttonGrid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);
    
    // Create system button
    createSystemButton();
    
    // Create WiFi button
    createWiFiButton();
    
    // Create Devices button (3rd button)
    createDevicesButton();
    
    // Create Effects button (4th button)
    createEffectsButton();
    
    Serial.println("[LVGL] Button grid created");
}

static void createSystemButton() {
    Serial.println("[LVGL] Creating system button...");
    
    // Create button
    lvgl_systemButton = lv_btn_create(lvgl_buttonGrid);
    lv_obj_set_size(lvgl_systemButton, LV_PCT(45), LV_PCT(45));
    
    // Style the button
    lv_obj_set_style_bg_color(lvgl_systemButton, lv_color_hex(0xE0E0E0), 0);  // Light gray
    lv_obj_set_style_border_width(lvgl_systemButton, 3, 0);
    lv_obj_set_style_border_color(lvgl_systemButton, lv_color_black(), 0);
    lv_obj_set_style_radius(lvgl_systemButton, 10, 0);
    
    // Create label with icon and text
    lvgl_systemButtonLabel = lv_label_create(lvgl_systemButton);
    lv_obj_set_style_text_align(lvgl_systemButtonLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lvgl_systemButtonLabel, lv_color_black(), 0);  // Black text for contrast
    lv_label_set_text(lvgl_systemButtonLabel, LV_SYMBOL_DRIVE "\nSystem\n\nUptime: 0d 0h 0m 0s");
    lv_obj_center(lvgl_systemButtonLabel);
    
    // Add click event handler
    lv_obj_add_event_cb(lvgl_systemButton, systemButtonEventHandler, LV_EVENT_CLICKED, nullptr);
    
    Serial.println("[LVGL] System button created");
}

static void createWiFiButton() {
    Serial.println("[LVGL] Creating WiFi button...");
    
    // Create button
    lvgl_wifiButton = lv_btn_create(lvgl_buttonGrid);
    lv_obj_set_size(lvgl_wifiButton, LV_PCT(45), LV_PCT(45));
    
    // Style the button
    lv_obj_set_style_bg_color(lvgl_wifiButton, lv_color_hex(0xE0E0E0), 0);  // Light gray
    lv_obj_set_style_border_width(lvgl_wifiButton, 3, 0);
    lv_obj_set_style_border_color(lvgl_wifiButton, lv_color_black(), 0);
    lv_obj_set_style_radius(lvgl_wifiButton, 10, 0);
    
    // Create label with icon and text
    lvgl_wifiButtonLabel = lv_label_create(lvgl_wifiButton);
    lv_obj_set_style_text_align(lvgl_wifiButtonLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lvgl_wifiButtonLabel, lv_color_black(), 0);  // Black text for contrast
    lv_label_set_text(lvgl_wifiButtonLabel, LV_SYMBOL_WIFI "\nWiFi");
    lv_obj_center(lvgl_wifiButtonLabel);
    
    // Add click event handler
    lv_obj_add_event_cb(lvgl_wifiButton, wifiButtonEventHandler, LV_EVENT_CLICKED, nullptr);
    
    Serial.println("[LVGL] WiFi button created");
}

static void createDevicesButton() {
    Serial.println("[LVGL] Creating Devices button...");
    
    // Create button
    lv_obj_t* devicesButton = lv_btn_create(lvgl_buttonGrid);
    lv_obj_set_size(devicesButton, LV_PCT(45), LV_PCT(45));
    
    // Style the button
    lv_obj_set_style_bg_color(devicesButton, lv_color_hex(0xE0E0E0), 0);  // Light gray
    lv_obj_set_style_border_width(devicesButton, 3, 0);
    lv_obj_set_style_border_color(devicesButton, lv_color_black(), 0);
    lv_obj_set_style_radius(devicesButton, 10, 0);
    
    // Create label with icon and text
    lv_obj_t* devicesButtonLabel = lv_label_create(devicesButton);
    lv_obj_set_style_text_align(devicesButtonLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(devicesButtonLabel, lv_color_black(), 0);  // Black text for contrast
    lv_label_set_text(devicesButtonLabel, LV_SYMBOL_LIST "\nDevices");
    lv_obj_center(devicesButtonLabel);
    
    // Add click event handler
    lv_obj_add_event_cb(devicesButton, devicesButtonEventHandler, LV_EVENT_CLICKED, nullptr);
    
    Serial.println("[LVGL] Devices button created");
}

static void createEffectsButton() {
    Serial.println("[LVGL] Creating Effects button...");
    
    // Create button
    lv_obj_t* effectsButton = lv_btn_create(lvgl_buttonGrid);
    lv_obj_set_size(effectsButton, LV_PCT(45), LV_PCT(45));
    
    // Style the button (same as other buttons)
    lv_obj_set_style_bg_color(effectsButton, lv_color_hex(0xE0E0E0), 0);  // Light gray
    lv_obj_set_style_border_width(effectsButton, 3, 0);
    lv_obj_set_style_border_color(effectsButton, lv_color_black(), 0);
    lv_obj_set_style_radius(effectsButton, 10, 0);
    
    // Create label with icon and text
    lv_obj_t* effectsButtonLabel = lv_label_create(effectsButton);
    lv_obj_set_style_text_align(effectsButtonLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(effectsButtonLabel, lv_color_black(), 0);  // Black text for contrast
    lv_label_set_text(effectsButtonLabel, LV_SYMBOL_PLAY "\nEffects");
    lv_obj_center(effectsButtonLabel);
    
    // Add click event handler
    lv_obj_add_event_cb(effectsButton, effectsButtonEventHandler, LV_EVENT_CLICKED, nullptr);
    
    Serial.println("[LVGL] Effects button created");
}

static void effectsButtonEventHandler(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    Serial.println("[LVGL] Effects button clicked - triggering next effect");
    TriggerNextEffect();  // Same as rotary encoder button
}

static void systemButtonEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[LVGL] System button clicked - showing task viewer");
        showTaskViewer();
    }
}

static void devicesButtonEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    Serial.printf("[LVGL] Devices button event: code=%d\n", code);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[LVGL] Devices button clicked - showing device management");
        showDeviceManagement();
    }
}

static void wifiButtonEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    Serial.printf("[LVGL] WiFi button event: code=%d\n", code);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[LVGL] WiFi button clicked - showing WiFi information");
        ShowWiFiInformation();
    } else if (code == LV_EVENT_PRESSED) {
        Serial.println("[LVGL] WiFi button pressed");
    } else if (code == LV_EVENT_RELEASED) {
        Serial.println("[LVGL] WiFi button released");
    }
}

void ShowWiFiInformation() {
    Serial.println("[LVGL] Showing WiFi information...");
    
    // If message box already exists, just update it
    if (lvgl_wifiMsgBox != nullptr) {
        updateWiFiMessageBox();
        return;
    }
    
    // Get screen dimensions
    lv_coord_t screenWidth = lv_obj_get_width(lvgl_screen);
    lv_coord_t screenHeight = lv_obj_get_height(lvgl_screen);
    
    // Calculate message box size (60% of screen)
    lv_coord_t msgBoxWidth = (screenWidth * 60) / 100;
    lv_coord_t msgBoxHeight = (screenHeight * 60) / 100;
    
    // Create backdrop (semi-transparent, mostly opaque)
    lvgl_wifiMsgBoxBackdrop = lv_obj_create(lvgl_screen);
    lv_obj_set_size(lvgl_wifiMsgBoxBackdrop, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(lvgl_wifiMsgBoxBackdrop, 0, 0);
    lv_obj_set_style_bg_color(lvgl_wifiMsgBoxBackdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lvgl_wifiMsgBoxBackdrop, LV_OPA_90, 0);  // 90% opaque
    lv_obj_set_style_border_opa(lvgl_wifiMsgBoxBackdrop, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(lvgl_wifiMsgBoxBackdrop, msgboxBackdropEventHandler, LV_EVENT_CLICKED, nullptr);
    lv_obj_move_foreground(lvgl_wifiMsgBoxBackdrop);
    
    // Create message box container
    lvgl_wifiMsgBox = lv_obj_create(lvgl_wifiMsgBoxBackdrop);
    lv_obj_set_size(lvgl_wifiMsgBox, msgBoxWidth, msgBoxHeight);
    lv_obj_center(lvgl_wifiMsgBox);
    lv_obj_set_style_bg_color(lvgl_wifiMsgBox, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lvgl_wifiMsgBox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lvgl_wifiMsgBox, 3, 0);
    lv_obj_set_style_border_color(lvgl_wifiMsgBox, lv_color_black(), 0);
    lv_obj_set_style_radius(lvgl_wifiMsgBox, 10, 0);
    lv_obj_set_style_pad_all(lvgl_wifiMsgBox, 20, 0);
    
    // Create content container
    lvgl_wifiMsgBoxContent = lv_obj_create(lvgl_wifiMsgBox);
    lv_obj_set_size(lvgl_wifiMsgBoxContent, LV_PCT(100), LV_PCT(80));
    lv_obj_set_pos(lvgl_wifiMsgBoxContent, 0, 0);
    lv_obj_set_style_bg_opa(lvgl_wifiMsgBoxContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(lvgl_wifiMsgBoxContent, LV_OPA_TRANSP, 0);
    
    // Create title
    lv_obj_t* title = lv_label_create(lvgl_wifiMsgBoxContent);
    lv_label_set_text(title, "WiFi Information");
    // Use default font (lv_font_montserrat_18 may not be available)
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create text label for WiFi info
    lvgl_wifiMsgBoxText = lv_label_create(lvgl_wifiMsgBoxContent);
    lv_obj_set_style_text_align(lvgl_wifiMsgBoxText, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(lvgl_wifiMsgBoxText, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_set_width(lvgl_wifiMsgBoxText, LV_PCT(90));
    
    // Create close button
    lvgl_wifiMsgBoxCloseBtn = lv_btn_create(lvgl_wifiMsgBox);
    lv_obj_set_size(lvgl_wifiMsgBoxCloseBtn, 100, 40);
    lv_obj_align(lvgl_wifiMsgBoxCloseBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(lvgl_wifiMsgBoxCloseBtn, lv_color_hex(0x808080), 0);
    lv_obj_add_event_cb(lvgl_wifiMsgBoxCloseBtn, msgboxCloseEventHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* closeBtnLabel = lv_label_create(lvgl_wifiMsgBoxCloseBtn);
    lv_label_set_text(closeBtnLabel, "Close");
    lv_obj_center(closeBtnLabel);
    
    // Initial content update
    updateWiFiMessageBox();
    lvgl_lastWifiMsgBoxUpdate = millis();
    
    Serial.println("[LVGL] WiFi message box created");
}

void updateWiFiMessageBox() {
    if (lvgl_wifiMsgBoxText == nullptr) {
        return;
    }
    
    // Get WiFi info
    String wifiInfo = "";
    String ipAddr = "";
    String status = "";
    
    auto* wifiMgr = TaskManager::getInstance().getWiFiManager();
    if (wifiMgr) {
        auto network_info = wifiMgr->getNetworkInfo();
        wifiInfo = network_info.ssid + " (" + String(network_info.rssi) + "dBm)";
        ipAddr = wifiMgr->getIPAddress();
        status = wifiMgr->getStatus();
    } else {
        wifiInfo = "(WiFi Manager not available)";
    }
    
    // Format message
    char msgText[256];
    snprintf(msgText, sizeof(msgText),
        "Network: %s\n"
        "IP Address: %s\n"
        "Status: %s",
        wifiInfo.c_str(),
        ipAddr.length() > 0 ? ipAddr.c_str() : "(not connected)",
        status.length() > 0 ? status.c_str() : "unknown");
    
    lv_label_set_text(lvgl_wifiMsgBoxText, msgText);
}

static void msgboxCloseEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[LVGL] Close button clicked");
        closeWiFiMessageBox();
    }
}

static void msgboxBackdropEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        // Only close if clicking the backdrop itself (not child objects)
        lv_obj_t* target = lv_event_get_target(e);
        if (target == lvgl_wifiMsgBoxBackdrop) {
            Serial.println("[LVGL] Backdrop clicked - closing message box");
            closeWiFiMessageBox();
        }
    }
}

void closeWiFiMessageBox() {
    if (lvgl_wifiMsgBoxBackdrop != nullptr) {
        lv_obj_del(lvgl_wifiMsgBoxBackdrop);
        lvgl_wifiMsgBoxBackdrop = nullptr;
        lvgl_wifiMsgBox = nullptr;
        lvgl_wifiMsgBoxContent = nullptr;
        lvgl_wifiMsgBoxCloseBtn = nullptr;
        lvgl_wifiMsgBoxText = nullptr;
        Serial.println("[LVGL] WiFi message box closed");
    }
}

void updateSystemButton() {
    if (lvgl_systemButtonLabel == nullptr) {
        return;
    }
    
    auto* sysMon = TaskManager::getInstance().getSystemMonitorTask();
    if (!sysMon) {
        return;
    }
    
    SystemStats stats = sysMon->getStats();
    uint32_t uptime = stats.uptimeSeconds;
    uint32_t days = uptime / 86400;
    uint32_t hours = (uptime % 86400) / 3600;
    uint32_t minutes = (uptime % 3600) / 60;
    uint32_t seconds = uptime % 60;
    
    char buttonText[128];
    snprintf(buttonText, sizeof(buttonText),
        LV_SYMBOL_DRIVE "\nSystem\n\nUptime: %u d %u h %u m %u s\nHeap: %d%%",
        (unsigned int)days, (unsigned int)hours,
        (unsigned int)minutes, (unsigned int)seconds,
        stats.heapUsagePercent);
    
    lv_label_set_text(lvgl_systemButtonLabel, buttonText);
}

#endif // PLATFORM_CROW_PANEL

