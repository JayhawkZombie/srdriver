#include "PlayerManager.h"
#include "../../Globals.h"
#include "../../freertos/LogManager.h"
#include "../../hal/ble/BLEManager.h"
#include "../../hal/SDCardController.h"
#include "../LightPlayer2.h"
#include "../blending/LayerStack.h"
#include "../blending/MainLayer.h"
#include "../blending/PatternLayer.h"

// Forward declarations for global variables
extern Light LightArr[NUM_LEDS];
extern Light BlendLightArr[NUM_LEDS];
extern Light FinalLeds[NUM_LEDS];
extern CRGB leds[];

PlayerManager::PlayerManager() 
    : initialized(false), rainbowPlayerEnabled(false), pulsePlayerEnabled(false),
      activePanelCount(4), panelType(2), currentRingPlayer(0), maxRingPlayers(4),
      configManager(nullptr), rainbowPlayer(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f, false) {
    
    // Initialize arrays
    ringPlayerEnabled.fill(false);
    lp2Data.fill(patternData{});
}

PlayerManager::~PlayerManager() {
    // Cleanup handled by RAII
}

void PlayerManager::setup(ConfigManager* configMgr) {
    configManager = configMgr;
    
    LOG_DEBUG("PlayerManager: Initializing players");
    
    initializePlayers();
    initializePanels();
    initializeLayers();
    initializeRingPlayers();
    
    // Load configuration from JSON if available
    loadConfigurationFromJSON();
    
    initialized = true;
    LOG_DEBUG("PlayerManager: Setup complete");
}

void PlayerManager::update(float dt) {
    if (!initialized) return;
    
    // Update all active players
    if (rainbowPlayerEnabled) {
        rainbowPlayer.update(dt);
    }
    
    if (pulsePlayerEnabled) {
        pulsePlayer.update(dt);
    }
    
    // Update active ring players
    for (int i = 0; i < 4; i++) {
        if (ringPlayerEnabled[i]) {
            ringPlayers[i].update(dt);
        }
    }
    
    // Update layer system
    if (layerStack) {
        layerStack->update(dt);
    }
    
    // Update panels
    for (int i = 0; i < activePanelCount; i++) {
        panels[i].update();
    }
}

WavePlayer* PlayerManager::getCurrentWavePlayer() {
    return &testWavePlayer;
}

void PlayerManager::switchWavePlayer(int index) {
    if (!configManager) {
        LOG_ERROR("PlayerManager: ConfigManager not available");
        return;
    }
    
    WavePlayerConfig* config = configManager->getWavePlayerConfig(index);
    if (!config) {
        LOG_ERRORF("PlayerManager: Invalid wave player config index: %d", index);
        return;
    }
    
    LOG_DEBUGF("PlayerManager: Switching to wave player %d: %s", index, config->name.c_str());
    
    testWavePlayer.nTermsLt = config->nTermsLt;
    testWavePlayer.nTermsRt = config->nTermsRt;
    testWavePlayer.init(LightArr[0], config->rows, config->cols, config->onLight, config->offLight);
    testWavePlayer.setWaveData(config->AmpRt, config->wvLenLt, config->wvSpdLt, config->wvLenRt, config->wvSpdRt);
    testWavePlayer.setRightTrigFunc(config->rightTrigFuncIndex);
    testWavePlayer.setLeftTrigFunc(config->leftTrigFuncIndex);
    
    // Set up coefficients
    float C_Rt[3] = {3, 2, 1};
    float C_Lt[3] = {3, 2, 1};
    float *rightCoeffs, *leftCoeffs;
    int nTermsRt, nTermsLt;
    
    // Use the same helper function as before
    extern void SetupWavePlayerCoefficients(const WavePlayerConfig &config, float *C_Rt, float *C_Lt, 
                                           float **rightCoeffs, float **leftCoeffs, int *nTermsRt, int *nTermsLt);
    SetupWavePlayerCoefficients(*config, C_Rt, C_Lt, &rightCoeffs, &leftCoeffs, &nTermsRt, &nTermsLt);
    testWavePlayer.setSeriesCoeffs_Unsafe(rightCoeffs, nTermsRt, leftCoeffs, nTermsLt);
}

void PlayerManager::updateWavePlayerColors(Light high, Light low) {
    testWavePlayer.hiLt = high;
    testWavePlayer.loLt = low;
    testWavePlayer.init(LightArr[0], testWavePlayer.rows, testWavePlayer.cols, high, low);
    
    // Update BLE characteristics
    extern void UpdateAllCharacteristicsForCurrentPattern();
    UpdateAllCharacteristicsForCurrentPattern();
}

std::pair<Light, Light> PlayerManager::getCurrentWavePlayerColors() {
    return std::make_pair(testWavePlayer.hiLt, testWavePlayer.loLt);
}

void PlayerManager::setRainbowPlayerEnabled(bool enabled) {
    rainbowPlayerEnabled = enabled;
    rainbowPlayer.setEnabled(enabled);
    LOG_DEBUGF("PlayerManager: Rainbow player %s", enabled ? "enabled" : "disabled");
    updateLayerComposition();
}

void PlayerManager::setRainbowPlayerSpeed(float speed) {
    rainbowPlayer.setSpeed(speed);
    LOG_DEBUGF("PlayerManager: Rainbow player speed set to %f", speed);
}

void PlayerManager::setRainbowPlayerDirection(bool reverse) {
    rainbowPlayer.setDirection(reverse);
    LOG_DEBUGF("PlayerManager: Rainbow player direction set to %s", reverse ? "reverse" : "forward");
}

bool PlayerManager::isRainbowPlayerEnabled() const {
    return rainbowPlayerEnabled;
}

void PlayerManager::setPulsePlayerEnabled(bool enabled) {
    pulsePlayerEnabled = enabled;
    LOG_DEBUGF("PlayerManager: Pulse player %s", enabled ? "enabled" : "disabled");
    updateLayerComposition();
}

void PlayerManager::setPulsePlayerParameters(float frequency, float amplitude, float phase, bool continuous) {
    // Reinitialize pulse player with new parameters
    pulsePlayer.init(BlendLightArr[0], 32, 32, Light(255, 255, 255), Light(0, 0, 0), 
                     frequency, amplitude, phase, continuous);
    LOG_DEBUGF("PlayerManager: Pulse player parameters updated - freq: %f, amp: %f, phase: %f", 
               frequency, amplitude, phase);
}

bool PlayerManager::isPulsePlayerEnabled() const {
    return pulsePlayerEnabled;
}

void PlayerManager::setRingPlayerEnabled(int index, bool enabled) {
    if (index < 0 || index >= 4) {
        LOG_ERRORF("PlayerManager: Invalid ring player index: %d", index);
        return;
    }
    
    ringPlayerEnabled[index] = enabled;
    if (enabled) {
        ringPlayers[index].Start();
        LOG_DEBUGF("PlayerManager: Ring player %d enabled", index);
    } else {
        ringPlayers[index].StopWave();
        LOG_DEBUGF("PlayerManager: Ring player %d disabled", index);
    }
}

void PlayerManager::setRingPlayerParameters(int index, float speed, float width, Light highColor, Light lowColor) {
    if (index < 0 || index >= 4) {
        LOG_ERRORF("PlayerManager: Invalid ring player index: %d", index);
        return;
    }
    
    ringPlayers[index].ringSpeed = speed;
    ringPlayers[index].ringWidth = width;
    ringPlayers[index].hiLt = highColor;
    ringPlayers[index].loLt = lowColor;
    
    LOG_DEBUGF("PlayerManager: Ring player %d parameters updated", index);
}

void PlayerManager::moveToNextRingPlayer() {
    ringPlayers[currentRingPlayer].StopWave();
    currentRingPlayer = (currentRingPlayer + 1) % maxRingPlayers;
    ringPlayers[currentRingPlayer].Start();
    LOG_DEBUGF("PlayerManager: Moved to ring player %d", currentRingPlayer);
}

RingPlayer* PlayerManager::getCurrentRingPlayer() {
    return &ringPlayers[currentRingPlayer];
}

int PlayerManager::getActiveRingPlayerCount() const {
    int count = 0;
    for (bool enabled : ringPlayerEnabled) {
        if (enabled) count++;
    }
    return count;
}

void PlayerManager::firePattern(int patternIndex, Light onColor, Light offColor) {
    if (patternIndex < 0 || patternIndex >= 40) {
        LOG_ERRORF("PlayerManager: Invalid pattern index: %d", patternIndex);
        return;
    }
    
    unsigned int playerIndex = findAvailablePatternPlayer();
    if (playerIndex == -1) {
        LOG_ERROR("PlayerManager: No available pattern player found");
        return;
    }
    
    LOG_DEBUGF("PlayerManager: Firing pattern %d on player %d", patternIndex, playerIndex);
    
    firedPatternPlayers[playerIndex].setToPlaySinglePattern(true);
    firedPatternPlayers[playerIndex].drawOffLt = false;
    firedPatternPlayers[playerIndex].onLt = onColor;
    firedPatternPlayers[playerIndex].offLt = offColor;
    firedPatternPlayers[playerIndex].firePattern(patternIndex);
}

unsigned int PlayerManager::findAvailablePatternPlayer() {
    for (unsigned int i = 0; i < firedPatternPlayers.size(); ++i) {
        if (!firedPatternPlayers[i].isPlayingSinglePattern()) {
            return i;
        }
    }
    return -1;
}

void PlayerManager::setLayerEnabled(int layerIndex, bool enabled) {
    // LayerStack doesn't have enable/disable methods yet
    // This would need to be implemented in LayerStack class
    LOG_DEBUGF("PlayerManager: Layer %d %s (not implemented yet)", layerIndex, enabled ? "enabled" : "disabled");
}

void PlayerManager::addCustomLayer(class Layer* layer) {
    if (layerStack) {
        // LayerStack addLayer method needs to be updated to accept Layer* instead of template
        LOG_DEBUG("PlayerManager: Custom layer addition (not implemented yet)");
    }
}

void PlayerManager::removeLayer(int layerIndex) {
    // LayerStack doesn't have remove methods yet
    LOG_DEBUGF("PlayerManager: Layer %d removal (not implemented yet)", layerIndex);
}

void PlayerManager::setPanelConfiguration(int panelCount, int panelType) {
    activePanelCount = panelCount;
    this->panelType = panelType;
    initializePanels();
    LOG_DEBUGF("PlayerManager: Panel configuration updated - count: %d, type: %d", panelCount, panelType);
}

void PlayerManager::setPanelRotation(int panelIndex, int rotation) {
    if (panelIndex >= 0 && panelIndex < activePanelCount) {
        panels[panelIndex].rotIdx = rotation;
        LOG_DEBUGF("PlayerManager: Panel %d rotation set to %d", panelIndex, rotation);
    }
}

void PlayerManager::loadConfigurationFromJSON() {
    if (!configManager) {
        LOG_WARN("PlayerManager: No ConfigManager available for JSON loading");
        setupDefaultConfiguration();
        return;
    }
    
    LOG_DEBUG("PlayerManager: Loading configuration from JSON");
    // This would load player configurations from JSON
    // For now, use defaults
    setupDefaultConfiguration();
}

void PlayerManager::saveConfigurationToJSON() {
    LOG_DEBUG("PlayerManager: Saving configuration to JSON");
    // This would save current player configurations to JSON
}

void PlayerManager::resetToDefaults() {
    LOG_DEBUG("PlayerManager: Resetting to default configuration");
    setupDefaultConfiguration();
}

void PlayerManager::initializePlayers() {
    // Rainbow player is already initialized in constructor
    
    // Initialize fired pattern players
    for (auto &player : firedPatternPlayers) {
        player.onLt = Light(255, 255, 255);
        player.offLt = Light(0, 0, 0);
        player.init(LightArr[0], 1, 120, lp2Data[0], 18);
        player.drawOffLt = false;
        player.setToPlaySinglePattern(true);
        player.update();
    }
}

void PlayerManager::initializePanels() {
    LOG_DEBUG("PlayerManager: Initializing panels");
    
    // Initialize panels for 2x2 configuration
    for (int i = 0; i < activePanelCount; i++) {
        panels[i].init_Src(FinalLeds, DIMS_PANELS, DIMS_PANELS);
        panels[i].type = panelType;
        panels[i].rotIdx = 0;
    }
    
    // Set up panel areas and targets
    panels[0].set_SrcArea(16, 16, 0, 0);
    panels[0].pTgt0 = leds;
    
    panels[1].set_SrcArea(16, 16, 0, 16);
    panels[1].pTgt0 = leds + 256;
    
    panels[2].set_SrcArea(16, 16, 16, 0);
    panels[2].pTgt0 = leds + 512;
    
    panels[3].set_SrcArea(16, 16, 16, 16);
    panels[3].pTgt0 = leds + 768;
    panels[3].rotIdx = 2; // Rotate 180 degrees
}

void PlayerManager::initializeLayers() {
    LOG_DEBUG("PlayerManager: Initializing layer system");
    
    layerStack = std::unique_ptr<LayerStack>(new LayerStack(NUM_LEDS));
    // Note: LayerStack addLayer method needs to be updated to work with our layer types
    // For now, we'll skip layer initialization until LayerStack is updated
    LOG_DEBUG("PlayerManager: Layer system initialization (LayerStack needs updates)");
}

void PlayerManager::initializeRingPlayers() {
    LOG_DEBUG("PlayerManager: Initializing ring players");
    
    // Initialize all ring players with default configurations
    for (int i = 0; i < 4; i++) {
        ringPlayers[i].initToGrid(FinalLeds, DIMS_PANELS, DIMS_PANELS);
        ringPlayers[i].setRingCenter(15.5f, 15.5f);
        ringPlayers[i].onePulse = false;
        ringPlayers[i].Amp = 1.0f;
    }
    
    // Set up default ring player configurations
    setRingPlayerParameters(0, 17.1f, 0.22f, Light(125, 0, 255), Light(0, 0, 0));
    setRingPlayerParameters(1, 70.0f, 0.46f, Light(0, 64, 255), Light(0, 0, 0));
    setRingPlayerParameters(2, 9.61f, 0.355f, Light(32, 255, 0), Light(0, 0, 0));
    setRingPlayerParameters(3, 10.3f, 2.5f, Light(0, 255, 255), Light(0, 32, 32));
    
    // Enable first ring player by default
    setRingPlayerEnabled(0, true);
}

void PlayerManager::setupDefaultConfiguration() {
    // Set default player states
    setRainbowPlayerEnabled(true);
    setRainbowPlayerSpeed(5.0f);
    setRainbowPlayerDirection(true);
    
    setPulsePlayerEnabled(true);
    setPulsePlayerParameters(220, 800.0f, 8.0f, true);
    
    // Enable first ring player
    setRingPlayerEnabled(0, true);
}

void PlayerManager::loadPlayerConfigurations() {
    // This would load specific player configurations from JSON
    // For now, use defaults
    setupDefaultConfiguration();
}

void PlayerManager::updateLayerComposition() {
    // Update layer composition based on enabled players
    if (layerStack) {
        // Rebuild layer stack based on current player states
        // This is where you'd dynamically add/remove layers based on what's enabled
    }
}
