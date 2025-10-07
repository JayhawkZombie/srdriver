#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <array>
#include "../Light.h"
#include "../LightPlayer2.h"
#include "../WavePlayer.h"

// Forward declarations
class SDCardController;

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // Main configuration loading
    bool loadPatterns();
    void loadWavePlayerConfigs();
    void loadRainbowPlayerConfigs();
    
    // Getters for configuration data
    WavePlayerConfig* getWavePlayerConfig(int index);
    std::vector<float> getWavePlayerSpeeds() const;
    int getNumWavePlayerConfigs() const;
    
    // Pattern data access
    void initializePatternData();
    
    // Status
    bool isLoaded() const { return patternsLoaded; }
    
private:
    // JSON document and configuration arrays
    DynamicJsonDocument patternsDoc;
    WavePlayerConfig jsonWavePlayerConfigs[12];
    std::vector<float> wavePlayerSpeeds;
    
    // Pattern data
    std::array<patternData, 40> lp2Data;
    
    // State
    bool patternsLoaded;
    bool waveConfigsLoaded;
    bool rainbowConfigsLoaded;
    
    // Helper methods
    bool loadPatternsFromSD();
    void parseWavePlayerConfigs();
    void parseRainbowPlayerConfigs();
};
