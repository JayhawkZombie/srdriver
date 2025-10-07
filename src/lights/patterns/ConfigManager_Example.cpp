// Example of how to use the new ConfigManager in your existing code
// This shows the minimal changes needed to start using it

#include "ConfigManager.h"

// Global instance (you can make this a singleton later)
ConfigManager* g_configManager = nullptr;

// Modified Pattern_Setup function - minimal changes
void Pattern_Setup_WithConfigManager() {
    // Create config manager
    g_configManager = new ConfigManager();
    
    // Load all patterns using the new manager
    if (g_configManager->loadPatterns()) {
        LOG_DEBUG("Pattern setup: All patterns loaded successfully");
    } else {
        LOG_ERROR("Pattern setup: Failed to load patterns");
        return;
    }
    
    // Get wave player speeds from config manager
    std::vector<float> speeds = g_configManager->getWavePlayerSpeeds();
    LOG_DEBUGF("Loaded %d wave player speeds", speeds.size());
    
    // Get specific config
    WavePlayerConfig* config = g_configManager->getWavePlayerConfig(1);
    if (config) {
        LOG_DEBUGF("Using config: %s", config->name.c_str());
    }
    
    // Rest of your existing setup code...
    // (rainbowPlayer setup, panels setup, etc.)
}

// Example of how to use in SwitchWavePlayerIndex
void SwitchWavePlayerIndex_WithConfigManager(int index) {
    if (!g_configManager) {
        LOG_ERROR("ConfigManager not initialized");
        return;
    }
    
    WavePlayerConfig* config = g_configManager->getWavePlayerConfig(index);
    if (!config) {
        LOG_ERRORF("Invalid config index: %d", index);
        return;
    }
    
    LOG_DEBUGF("Switching to config %d: %s", index, config->name.c_str());
    
    // Use the config to set up your wave player
    // (your existing SwitchWavePlayerIndex logic, but using config from manager)
}
