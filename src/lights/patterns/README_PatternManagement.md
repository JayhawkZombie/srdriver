# Pattern Management System Refactoring

## Overview

The current PatternManager system is a large, monolithic collection of global functions and variables that manages LED choreography. This document outlines a plan to refactor it into a clean, object-oriented architecture with proper separation of concerns.

## Current Problems

- **Monolithic Structure**: 1200+ lines of code in a single file
- **Global State**: Heavy reliance on global variables and extern declarations
- **Tight Coupling**: All components are interdependent
- **Hard to Test**: No clear interfaces or dependency injection
- **Difficult to Extend**: Adding new features requires modifying core logic

## Proposed Class Structure

### 1. **PatternManager** (Main Coordinator)
**Purpose**: High-level coordinator that orchestrates all LED patterns

**Responsibilities**:
- Pattern switching and navigation (`GoToPattern`, `GoToNextPattern`)
- BLE command handling (`ParseAndExecuteCommand`, `ParseFirePatternCommand`)
- Overall system state management
- Delegating to specialized managers
- User preference management (`ApplyFromUserPreferences`, `SaveUserPreferences`)

**Key Methods**:
```cpp
class PatternManager {
public:
    void setup();
    void update();
    void goToPattern(int index);
    void goToNextPattern();
    void handleBLECommand(const String& command);
    void applyUserPreferences(const DeviceState& state);
    void saveUserPreferences();
    
private:
    PlayerManager* playerManager;
    ConfigManager* configManager;
    AlertManager* alertManager;
    PanelManager* panelManager;
    LayerManager* layerManager;
};
```

### 2. **PlayerManager** (LED Player Management)
**Purpose**: Manages all LED players and their lifecycle

**Responsibilities**:
- WavePlayer management (`testWavePlayer`, `alertWavePlayer`)
- RainbowPlayer management (`rainbowPlayer`)
- RingPlayer management (`unusualRingPlayers` array)
- PulsePlayer management
- LightPlayer2 pattern firing system (`firedPatternPlayers`)
- Player state tracking and updates
- Pattern color management

**Key Methods**:
```cpp
class PlayerManager {
public:
    void update(float dt);
    WavePlayer* getCurrentWavePlayer();
    void switchWavePlayer(int index);
    void firePattern(int idx, Light on, Light off);
    void setAlertPlayer(const String& reason);
    void stopAlertPlayer();
    void updatePatternColors(Light high, Light low);
    std::pair<Light, Light> getCurrentPatternColors();
    
private:
    WavePlayer testWavePlayer;
    WavePlayer alertWavePlayer;
    RainbowPlayer rainbowPlayer;
    RingPlayer ringPlayers[4];
    std::array<LightPlayer2, 40> firedPatternPlayers;
    PulsePlayer pulsePlayer;
};
```

### 3. **ConfigManager** (Configuration & JSON)
**Purpose**: Handles all configuration loading and management

**Responsibilities**:
- JSON pattern loading from SD card (`LoadPatternsFromJson`)
- WavePlayerConfig management (`LoadWavePlayerConfigsFromJsonDocument`)
- RainbowPlayerConfig management (`LoadRainbowPlayerConfigsFromJsonDocument`)
- Pattern data initialization (`lp2Data` array)
- Configuration validation and error handling
- Speed management (`wavePlayerSpeeds`)

**Key Methods**:
```cpp
class ConfigManager {
public:
    bool loadPatterns();
    void loadWavePlayerConfigs();
    void loadRainbowPlayerConfigs();
    WavePlayerConfig* getWavePlayerConfig(int index);
    std::vector<float> getWavePlayerSpeeds();
    void initializePatternData();
    
private:
    DynamicJsonDocument patternsDoc;
    WavePlayerConfig jsonWavePlayerConfigs[12];
    std::array<patternData, 40> lp2Data;
    std::vector<float> wavePlayerSpeeds;
};
```

### 4. **AlertManager** (Thermal & Alert System)
**Purpose**: Manages alert overlays and thermal management

**Responsibilities**:
- Alert wave player management (`SetAlertWavePlayer`, `StopAlertWavePlayer`)
- Thermal brightness reduction
- Alert blending with main patterns (`BlendWavePlayers`)
- Alert state tracking
- Temperature-based brightness curves

**Key Methods**:
```cpp
class AlertManager {
public:
    void setAlert(const String& reason);
    void stopAlert();
    void update(float dt);
    void blendWithMainPattern();
    bool isAlertActive() const;
    float getThermalBrightnessCurve(float currentBrightness);
    
private:
    WavePlayer alertWavePlayer;
    Light alertLightArr[NUM_LEDS];
    bool alertActive;
    String alertReason;
};
```

### 5. **PanelManager** (LED Panel Transformations)
**Purpose**: Manages LED panel layout and transformations

**Responsibilities**:
- LightPanel array management (2x2 configuration)
- Panel transformations and mapping
- LED array coordinate mapping
- Panel update coordination
- Ring player management

**Key Methods**:
```cpp
class PanelManager {
public:
    void setup();
    void update();
    void moveToNextRingPlayer();
    RingPlayer* getCurrentRingPlayer();
    void setPanelRotation(int panelIndex, int rotation);
    
private:
    LightPanel panels[4];
    RingPlayer ringPlayers[4];
    int playingRingPlayer;
    int maxRingPlayers;
};
```

### 6. **LayerManager** (Blending System)
**Purpose**: Manages the layer blending system

**Responsibilities**:
- LayerStack management
- Layer composition and ordering
- Blending mode coordination
- Final LED output rendering
- Layer enable/disable management

**Key Methods**:
```cpp
class LayerManager {
public:
    void setup();
    void update(float dt);
    void render(Light* output);
    void addLayer(Layer* layer);
    void removeLayer(int index);
    void setLayerEnabled(int index, bool enabled);
    
private:
    std::unique_ptr<LayerStack> layerStack;
    Light finalLeds[NUM_LEDS];
    Light blendLightArr[NUM_LEDS];
};
```

## Migration Strategy

### Phase 1: Create New Structure
- Create new class files alongside existing code
- Implement basic class structure with minimal functionality
- Ensure compilation without breaking existing code

### Phase 2: Gradual Migration
- Move functionality piece by piece into new classes
- Update function calls to use new classes
- Maintain backward compatibility during transition

### Phase 3: Update Integration
- Update main.cpp to use new PatternManager
- Replace global function calls with class method calls
- Update BLE integration

### Phase 4: Cleanup
- Remove old global functions and variables
- Clean up extern declarations
- Optimize and refactor

### Phase 5: Testing & Validation
- Ensure all functionality works as before
- Add unit tests for individual components
- Performance testing and optimization

## Benefits of New Structure

1. **Single Responsibility**: Each class has a clear, focused purpose
2. **Loose Coupling**: Classes interact through well-defined interfaces
3. **Testability**: Each component can be tested independently
4. **Maintainability**: Changes to one system don't affect others
5. **Extensibility**: Easy to add new player types or managers
6. **Memory Management**: Better control over object lifecycle
7. **Debugging**: Easier to isolate issues to specific components

## Implementation Notes

- Use dependency injection for manager dependencies
- Implement proper RAII for resource management
- Add comprehensive error handling and logging
- Consider using smart pointers for automatic memory management
- Maintain thread safety for FreeRTOS integration

## File Organization

```
src/lights/patterns/
├── PatternManager.h/cpp          # Main coordinator
├── PlayerManager.h/cpp           # LED player management
├── ConfigManager.h/cpp           # Configuration handling
├── AlertManager.h/cpp            # Alert system
├── PanelManager.h/cpp             # Panel transformations
├── LayerManager.h/cpp             # Blending system
└── README_PatternManagement.md   # This documentation
```

This refactoring will transform the monolithic PatternManager into a clean, maintainable, and extensible system that supports your LED choreography development goals.
