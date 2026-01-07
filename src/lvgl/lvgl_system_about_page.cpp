#include "lvgl_system_about_page.h"
#include "freertos/LogManager.h"
#include "DeviceInfo.h"
#include "hal/PlatformFactory.h"
#include "version.h"

#if PLATFORM_CROW_PANEL

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

lv_obj_t* createSystemAboutPage(lv_obj_t* menu) {
    LOG_DEBUG_COMPONENT("LVGL", "Creating About page...");
    
    // Create page
    lv_obj_t* page = lv_menu_page_create(menu, nullptr);
    lv_obj_t* header = lv_menu_get_main_header(menu);
    lv_obj_set_style_pad_hor(page, lv_obj_get_style_pad_left(header, LV_PART_MAIN), 0);
    
    // Add separator
    lv_menu_separator_create(page);
    
    // ===== Platform Section =====
    lv_obj_t* platformSection = lv_menu_section_create(page);
    
    // Section title
    createTextMenuItem(platformSection, nullptr, "Platform", nullptr);
    
    // Platform name
    createTextMenuItem(platformSection, nullptr, "Board:", PlatformFactory::getPlatformName());
    
    // CPU frequency
    char cpuFreqStr[32];
    snprintf(cpuFreqStr, sizeof(cpuFreqStr), "%d MHz", PlatformFactory::getCpuFreqMHz());
    createTextMenuItem(platformSection, nullptr, "CPU:", cpuFreqStr);
    
    // Memory info (free/total)
    char memStr[64];
    uint32_t freeHeap = PlatformFactory::getFreeHeap();
    uint32_t totalHeap = PlatformFactory::getHeapSize();
    snprintf(memStr, sizeof(memStr), "%u / %u bytes", freeHeap, totalHeap);
    createTextMenuItem(platformSection, nullptr, "Memory:", memStr);
    
    // ===== Firmware Section =====
    lv_menu_separator_create(page);
    lv_obj_t* firmwareSection = lv_menu_section_create(page);
    
    // Section title
    createTextMenuItem(firmwareSection, nullptr, "Firmware", nullptr);
    
    // Firmware version
    createTextMenuItem(firmwareSection, nullptr, "Version:", DeviceInfo::GetCompiledFirmwareVersion().c_str());
    
    // Build date/time
    createTextMenuItem(firmwareSection, nullptr, "Build Date:", DeviceInfo::getBuildDate().c_str());
    createTextMenuItem(firmwareSection, nullptr, "Build Time:", DeviceInfo::getBuildTime().c_str());
    
    // Version branch
    createTextMenuItem(firmwareSection, nullptr, "Branch:", DeviceInfo::getVersionBranch().c_str());
    
    // Version hash
    char hashStr[32];
    snprintf(hashStr, sizeof(hashStr), "#%s", VERSION_HASH);
    createTextMenuItem(firmwareSection, nullptr, "Commit:", hashStr);
    
    // Device name/version
    createTextMenuItem(firmwareSection, nullptr, "Device:", DeviceInfo::getDeviceName().c_str());
    createTextMenuItem(firmwareSection, nullptr, "HW Version:", DeviceInfo::getDeviceVersion().c_str());
    
    // ===== Capabilities Section =====
    lv_menu_separator_create(page);
    lv_obj_t* capabilitiesSection = lv_menu_section_create(page);
    
    // Section title
    createTextMenuItem(capabilitiesSection, nullptr, "Capabilities", nullptr);
    
    // List capabilities
    const auto& capabilities = DeviceInfo::getCapabilities();
    if (capabilities.empty()) {
        createTextMenuItem(capabilitiesSection, nullptr, "(None)", nullptr);
    } else {
        for (const auto& capability : capabilities) {
            createTextMenuItem(capabilitiesSection, LV_SYMBOL_OK, capability.c_str(), nullptr);
        }
    }
    
    LOG_DEBUG_COMPONENT("LVGL", "About page created");
    return page;
}

#endif // PLATFORM_CROW_PANEL

