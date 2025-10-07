#include "ConfigManager.h"
#include "../../hal/SDCardController.h"
#include "../../freertos/LogManager.h"
#include "../../Globals.h"
#include "../LightPlayer2.h"

ConfigManager::ConfigManager() 
    : patternsDoc(8196 * 8), patternsLoaded(false), waveConfigsLoaded(false), rainbowConfigsLoaded(false) {
    // Initialize pattern data array
    lp2Data.fill(patternData{});
}

ConfigManager::~ConfigManager() {
    // Cleanup handled by RAII
}

bool ConfigManager::loadPatterns() {
    LOG_DEBUG("ConfigManager: Loading patterns from JSON");
    
    if (!loadPatternsFromSD()) {
        LOG_ERROR("ConfigManager: Failed to load patterns from SD card");
        return false;
    }
    
    loadWavePlayerConfigs();
    loadRainbowPlayerConfigs();
    initializePatternData();
    
    patternsLoaded = true;
    LOG_DEBUG("ConfigManager: All patterns loaded successfully");
    return true;
}

bool ConfigManager::loadPatternsFromSD() {
    extern SDCardController *g_sdCardController;
    
    if (!g_sdCardController || !g_sdCardController->isAvailable()) {
        LOG_ERROR("ConfigManager: SD card not available");
        return false;
    }
    
    LOG_DEBUG("ConfigManager: Loading patterns from /data/patterns.json");
    String patternsJson = g_sdCardController->readFile("/data/patterns.json");
    
    DeserializationError error = deserializeJson(patternsDoc, patternsJson);
    if (error) {
        LOG_ERRORF("ConfigManager: Failed to deserialize patterns JSON: %s", error.c_str());
        return false;
    }
    
    LOG_DEBUG("ConfigManager: Patterns JSON loaded successfully");
    return true;
}

void ConfigManager::loadWavePlayerConfigs() {
    LOG_DEBUG("ConfigManager: Loading wave player configs");
    
    if (patternsDoc.isNull()) {
        LOG_ERROR("ConfigManager: Patterns document is null");
        return;
    }
    
    JsonArray wavePlayerConfigsArray = patternsDoc["wavePlayerConfigs"];
    if (wavePlayerConfigsArray.isNull()) {
        LOG_ERROR("ConfigManager: Wave player configs array is null");
        return;
    }
    
    wavePlayerSpeeds.clear();
    
    for (int i = 0; i < wavePlayerConfigsArray.size() && i < 12; i++) {
        LOG_DEBUGF("ConfigManager: Loading wave player config %d", i);
        const JsonObject &config = wavePlayerConfigsArray[i];
        WavePlayerConfig &wpConfig = jsonWavePlayerConfigs[i];
        
        // Parse configuration
        wpConfig.name = config["name"].as<String>();
        wpConfig.rows = config["rows"].as<int>();
        wpConfig.cols = config["cols"].as<int>();
        wpConfig.onLight = Light(config["onLight"]["r"].as<int>(), 
                                config["onLight"]["g"].as<int>(), 
                                config["onLight"]["b"].as<int>());
        wpConfig.offLight = Light(config["offLight"]["r"].as<int>(), 
                                 config["offLight"]["g"].as<int>(), 
                                 config["offLight"]["b"].as<int>());
        wpConfig.AmpRt = config["AmpRt"].as<float>();
        wpConfig.wvLenLt = config["wvLenLt"].as<float>();
        wpConfig.wvLenRt = config["wvLenRt"].as<float>();
        wpConfig.wvSpdLt = config["wvSpdLt"].as<float>();
        wpConfig.wvSpdRt = config["wvSpdRt"].as<float>();
        wpConfig.rightTrigFuncIndex = config["rightTrigFuncIndex"].as<int>();
        wpConfig.leftTrigFuncIndex = config["leftTrigFuncIndex"].as<int>();
        wpConfig.useRightCoefficients = config["useRightCoefficients"].as<bool>();
        wpConfig.useLeftCoefficients = config["useLeftCoefficients"].as<bool>();
        wpConfig.nTermsRt = config["nTermsRt"].as<int>();
        wpConfig.nTermsLt = config["nTermsLt"].as<int>();
        wpConfig.speed = config["speed"].as<float>();
        
        wavePlayerSpeeds.push_back(wpConfig.speed);
        
        // Parse coefficients
        for (int j = 0; j < 3; j++) {
            wpConfig.C_Rt[j] = config["C_Rt"][j].as<float>();
            wpConfig.C_Lt[j] = config["C_Lt"][j].as<float>();
        }
        
        wpConfig.setCoefficients(wpConfig.C_Rt, wpConfig.C_Lt);
        
        LOG_DEBUGF("ConfigManager: Loaded wave player config %d: %s", i, wpConfig.name.c_str());
    }
    
    waveConfigsLoaded = true;
    LOG_DEBUG("ConfigManager: Wave player configs loaded");
}

void ConfigManager::loadRainbowPlayerConfigs() {
    LOG_DEBUG("ConfigManager: Loading rainbow player configs");
    
    if (patternsDoc.isNull()) {
        LOG_ERROR("ConfigManager: Patterns document is null");
        return;
    }
    
    JsonArray rainbowPlayerConfigsArray = patternsDoc["rainbowPlayerConfigs"];
    if (rainbowPlayerConfigsArray.isNull()) {
        LOG_ERROR("ConfigManager: Rainbow player configs array is null");
        return;
    }
    
    int configCount = rainbowPlayerConfigsArray.size();
    LOG_DEBUGF("ConfigManager: Found %d rainbow player configs", configCount);
    
    if (configCount <= 0) {
        LOG_WARN("ConfigManager: No rainbow player configs found");
        return;
    }
    
    // Load configuration for each rainbow player (max 2)
    for (int i = 0; i < configCount && i < 2; i++) {
        LOG_DEBUGF("ConfigManager: Loading rainbow player config %d", i);
        
        const JsonObject &config = rainbowPlayerConfigsArray[i];
        if (config.isNull()) {
            LOG_ERRORF("ConfigManager: Rainbow player config %d is null", i);
            continue;
        }
        
        if (!config.containsKey("name") || !config.containsKey("enabled")) {
            LOG_ERRORF("ConfigManager: Rainbow player config %d missing required fields", i);
            continue;
        }
        
        String name = config["name"].as<String>();
        bool enabled = config["enabled"].as<bool>();
        
        LOG_DEBUGF("ConfigManager: Rainbow Player %d: %s, enabled: %s", 
                   i, name.c_str(), enabled ? "true" : "false");
    }
    
    rainbowConfigsLoaded = true;
    LOG_DEBUG("ConfigManager: Rainbow player configs loaded");
}

void ConfigManager::initializePatternData() {
    LOG_DEBUG("ConfigManager: Initializing pattern data");
    
    // Initialize pattern data array (moved from Pattern_Setup)
    lp2Data[0].init(1, 1, 2);
    lp2Data[1].init(2, 1, 2);
    lp2Data[2].init(3, 1, 10);
    lp2Data[3].init(4, 1, 10);
    lp2Data[4].init(5, 1, 8);
    lp2Data[5].init(6, 1, 10);
    lp2Data[6].init(7, 2, 10);
    lp2Data[7].init(10, 2, 8);
    lp2Data[8].init(11, 2, 8);
    lp2Data[9].init(12, 2, 8);
    lp2Data[10].init(13, 2, 8);
    lp2Data[11].init(14, 2, 10);
    lp2Data[12].init(15, 2, 10);
    lp2Data[13].init(16, 2, 10);
    lp2Data[14].init(31, 2, 10);
    lp2Data[15].init(32, 2, 10);
    lp2Data[16].init(33, 2, 10);
    lp2Data[17].init(34, 2, 8);
    lp2Data[18].init(80, 2, 8);
    lp2Data[19].init(40, 1, 8);
    
    LOG_DEBUG("ConfigManager: Pattern data initialized");
}

WavePlayerConfig* ConfigManager::getWavePlayerConfig(int index) {
    if (index < 0 || index >= 12) {
        LOG_ERRORF("ConfigManager: Invalid wave player config index: %d", index);
        return nullptr;
    }
    
    if (!waveConfigsLoaded) {
        LOG_ERROR("ConfigManager: Wave player configs not loaded");
        return nullptr;
    }
    
    return &jsonWavePlayerConfigs[index];
}

std::vector<float> ConfigManager::getWavePlayerSpeeds() const {
    return wavePlayerSpeeds;
}

int ConfigManager::getNumWavePlayerConfigs() const {
    return waveConfigsLoaded ? 12 : 0;
}
