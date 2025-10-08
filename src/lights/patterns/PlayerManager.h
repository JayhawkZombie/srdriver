#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <array>
#include <vector>
#include <memory>
#include "../Light.h"
#include "../WavePlayer.h"
#include "../RainbowPlayer.h"
#include "../PulsePlayer.h"
#include "../RingPlayer.h"
#include "../LightPlayer2.h"
#include "../LightPanel.h"
#include "../blending/LayerStack.h"
#include "../blending/MainLayer.h"
#include "../blending/PatternLayer.h"
#include "ConfigManager.h"

// Forward declarations
class ConfigManager;

class PlayerManager {
public:
    PlayerManager();
    ~PlayerManager();
    
    // Initialization
    void setup(ConfigManager* configManager);
    void update(float dt);
    
    // Wave Player Management
    WavePlayer* getCurrentWavePlayer();
    void switchWavePlayer(int index);
    void updateWavePlayerColors(Light high, Light low);
    std::pair<Light, Light> getCurrentWavePlayerColors();
    
    // Rainbow Player Management
    void setRainbowPlayerEnabled(bool enabled);
    void setRainbowPlayerSpeed(float speed);
    void setRainbowPlayerDirection(bool reverse);
    bool isRainbowPlayerEnabled() const;
    
    // Pulse Player Management
    void setPulsePlayerEnabled(bool enabled);
    void setPulsePlayerParameters(float frequency, float amplitude, float phase, bool continuous);
    bool isPulsePlayerEnabled() const;
    
    // Ring Player Management
    void setRingPlayerEnabled(int index, bool enabled);
    void setRingPlayerParameters(int index, float speed, float width, Light highColor, Light lowColor);
    void moveToNextRingPlayer();
    RingPlayer* getCurrentRingPlayer();
    int getActiveRingPlayerCount() const;
    
    // Pattern Firing
    void firePattern(int patternIndex, Light onColor, Light offColor);
    unsigned int findAvailablePatternPlayer();
    
    // Layer Management
    void setLayerEnabled(int layerIndex, bool enabled);
    void addCustomLayer(class Layer* layer);
    void removeLayer(int layerIndex);
    
    // Panel Management
    void setPanelConfiguration(int panelCount, int panelType);
    void setPanelRotation(int panelIndex, int rotation);
    
    // Runtime Configuration
    void loadConfigurationFromJSON();
    void saveConfigurationToJSON();
    void resetToDefaults();
    
    // Status
    bool isInitialized() const { return initialized; }
    
private:
    // Core players
    WavePlayer testWavePlayer;
    WavePlayer alertWavePlayer;
    RainbowPlayer rainbowPlayer;
    PulsePlayer pulsePlayer;
    std::array<RingPlayer, 4> ringPlayers;
    std::array<LightPlayer2, 40> firedPatternPlayers;
    std::array<patternData, 40> lp2Data;
    
    // Panel system
    std::array<LightPanel, 4> panels;
    int activePanelCount;
    int panelType;
    
    // Layer system
    std::unique_ptr<LayerStack> layerStack;
    std::vector<std::unique_ptr<class Layer>> customLayers;
    
    // State
    bool initialized;
    bool rainbowPlayerEnabled;
    bool pulsePlayerEnabled;
    std::array<bool, 4> ringPlayerEnabled;
    int currentRingPlayer;
    int maxRingPlayers;
    
    // Configuration
    ConfigManager* configManager;
    
    // Helper methods
    void initializePlayers();
    void initializePanels();
    void initializeLayers();
    void initializeRingPlayers();
    void setupDefaultConfiguration();
    void loadPlayerConfigurations();
    void updateLayerComposition();
};
