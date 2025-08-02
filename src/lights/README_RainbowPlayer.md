# RainbowPlayer

A rainbow effect player that creates smooth rainbow animations using FastLED's native HSV color space, designed to work with the existing LED pattern system.

## Features

- **Delta Time Updates**: Uses real delta time for smooth, frame-rate independent animation
- **FastLED Native HSV**: Uses FastLED's efficient `CHSV` to `CRGB` conversion (0-255 hue range)
- **Configurable Range**: Can target specific LED ranges (startLED to endLED)
- **Adjustable Speed**: Control the rotation speed of the rainbow
- **Direction Control**: Flip the rainbow flow direction (start→end or end→start)
- **Hue Control**: Set the base hue of the rainbow effect (0-255)
- **Parameter Validation**: Automatically validates and corrects invalid parameters
- **High Performance**: Direct `CHSV` assignment for maximum efficiency

## Usage

### Basic Setup

```cpp
#include "RainbowPlayer.h"

// Create a rainbow player for all LEDs (flows from start to end)
RainbowPlayer rainbowPlayer(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f, false);

// Create a rainbow player that flows from end to start
RainbowPlayer reverseRainbow(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f, true);
```

### Constructor Parameters

```cpp
RainbowPlayer(Light* leds, int numLEDs, int startLED, int endLED, float speed = 1.0f, bool reverseDirection = false)
```

- `leds`: Pointer to the LED array (Light objects)
- `numLEDs`: Total number of LEDs in the array
- `startLED`: First LED index to include in the rainbow
- `endLED`: Last LED index to include in the rainbow
- `speed`: Rotation speed in rotations per second (default: 1.0)
- `reverseDirection`: If true, rainbow flows from end to start (default: false)

### Update Loop

```cpp
void UpdatePattern()
{
    static unsigned long lastUpdateTime = 0;
    const auto now = millis();
    const auto dt = now - lastUpdateTime;
    float dtSeconds = dt * 0.0001f;
    lastUpdateTime = now;
    
    // Update rainbow player
    rainbowPlayer.update(dtSeconds);
}
```

### Control Methods

```cpp
// Change speed (rotations per second)
rainbowPlayer.setSpeed(0.5f);  // Slow, gentle flow
rainbowPlayer.setSpeed(1.0f);  // Moderate speed (default)
rainbowPlayer.setSpeed(2.0f);  // Fast, energetic flow
rainbowPlayer.setSpeed(5.0f);  // Very fast, party mode
rainbowPlayer.setSpeed(-1.0f); // Reverse direction

// Change flow direction
rainbowPlayer.setDirection(false); // Normal: start → end
rainbowPlayer.setDirection(true);  // Reverse: end → start

// Set base hue (0-255, FastLED's native range)
rainbowPlayer.setHue(0);       // Red
rainbowPlayer.setHue(85);      // Green (255/3)
rainbowPlayer.setHue(170);     // Blue (255*2/3)
rainbowPlayer.setHue(255);     // Back to red

// Change LED range
rainbowPlayer.setStartLED(10);
rainbowPlayer.setEndLED(50);

// Change LED array
rainbowPlayer.setLEDs(newLightArray);
rainbowPlayer.setNumLEDs(newNumLEDs);
```

## Direction Control

The rainbow can flow in two directions:

### Normal Direction (start → end)
```cpp
RainbowPlayer rainbow(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f, false);
// or
RainbowPlayer rainbow(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f); // false is default
```
- Rainbow starts at the beginning of the strip and flows toward the end
- Red → Orange → Yellow → Green → Blue → Purple flowing down the strip

### Reverse Direction (end → start)
```cpp
RainbowPlayer rainbow(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f, true);
```
- Rainbow starts at the end of the strip and flows toward the beginning
- Red → Orange → Yellow → Green → Blue → Purple flowing up the strip

### Dynamic Direction Change
```cpp
// Change direction at runtime
rainbowPlayer.setDirection(true);  // Reverse direction
rainbowPlayer.setDirection(false); // Normal direction
```

## FastLED HSV System

The RainbowPlayer uses FastLED's native HSV system:
- **Hue**: 0-255 (not 0-360 degrees)
- **Saturation**: 0-255 (255 = full saturation)
- **Value**: 0-255 (255 = full brightness)

### Hue Values Reference
- `0` or `255`: Red
- `85`: Green
- `170`: Blue
- `43`: Orange
- `128`: Cyan
- `213`: Magenta

## Integration with Pattern System

### Adding to PatternManager

1. Add a global RainbowPlayer instance:
```cpp
RainbowPlayer g_rainbowPlayer(nullptr, 0, 0, 0, 1.0f, false);
```

2. Initialize in setup:
```cpp
void Pattern_Setup()
{
    // ... existing setup code ...
    g_rainbowPlayer = RainbowPlayer(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f, false);
}
```

3. Update in the main pattern loop:
```cpp
void UpdatePattern()
{
    // ... existing pattern code ...
    
    // Update rainbow player
    g_rainbowPlayer.update(dtSeconds);
    
    // ... rest of pattern code ...
}
```

### Multiple Rainbow Players

You can create multiple rainbow players for different sections:

```cpp
// Create rainbow on first third (normal direction)
RainbowPlayer section1(LightArr, NUM_LEDS, 0, NUM_LEDS/3 - 1, 0.5f, false);

// Create rainbow on middle third (reverse direction)
RainbowPlayer section2(LightArr, NUM_LEDS, NUM_LEDS/3, 2*NUM_LEDS/3 - 1, 1.5f, true);

// Create rainbow on last third (normal direction)
RainbowPlayer section3(LightArr, NUM_LEDS, 2*NUM_LEDS/3, NUM_LEDS - 1, 0.8f, false);

// Update all in your loop
section1.update(dtSeconds);
section2.update(dtSeconds);
section3.update(dtSeconds);
```

## Speed Guidelines

- `0.1f` - Very slow, barely visible movement
- `0.5f` - Slow, gentle flow
- `1.0f` - Moderate speed, good for ambient effects (default)
- `2.0f` - Fast, energetic movement
- `5.0f` - Very fast, party mode
- `10.0f` - Extremely fast, strobe-like effect
- Negative values - Reverse direction

## Performance Benefits

- **Direct CHSV Assignment**: Uses `CRGB rgbColor = CHSV(hue, 255, 255)` for maximum efficiency
- **No Manual Conversion**: Eliminates the need for `hsv2rgb_rainbow()` function calls
- **uint8_t Math**: Uses FastLED's native 0-255 hue range for optimal performance
- **Minimal Memory**: Only stores current hue and parameters
- **Fast Update Loop**: Suitable for real-time applications

## Example Patterns

### Simple Rainbow
```cpp
RainbowPlayer rainbow(LightArr, NUM_LEDS, 0, NUM_LEDS - 1, 1.0f, false);
rainbow.update(dtSeconds);
```

### Rainbow Wave
```cpp
// Create multiple rainbow players with different speeds and directions
for (int i = 0; i < 4; i++) {
    int start = i * NUM_LEDS / 4;
    int end = (i + 1) * NUM_LEDS / 4 - 1;
    float speed = 0.5f + i * 0.5f;
    bool reverse = (i % 2 == 1); // Alternate directions
    RainbowPlayer section(LightArr, NUM_LEDS, start, end, speed, reverse);
    section.update(dtSeconds);
}
```

### Rainbow Pulse
```cpp
// Vary speed over time for pulsing effect
float pulseSpeed = 1.0f + 0.5f * sin(millis() * 0.002f);
rainbowPlayer.setSpeed(pulseSpeed);
rainbowPlayer.update(dtSeconds);
```

### Direction Switching
```cpp
// Switch direction every 2 seconds
static unsigned long lastDirectionChange = 0;
if (millis() - lastDirectionChange > 2000) {
    lastDirectionChange = millis();
    rainbowPlayer.setDirection(!rainbowPlayer.getDirection()); // Toggle direction
}
```

### Direct FastLED Comparison
```cpp
// This is what RainbowPlayer does internally:
for (int i = 0; i < numLEDs; i++) {
    uint8_t hue = baseHue + (i * 10);
    leds[i] = CHSV(hue, 255, 255);
}
``` 