# LED Blending and Layer System

This system provides a flexible, shader-like approach to combining LED patterns and effects using layers and blending modes.

## Overview

The blending system consists of three main components:
1. **LEDBlender** - Defines how two colors are combined
2. **Layer** - Represents a pattern or effect that can be rendered
3. **LayerStack** - Manages multiple layers and renders them in sequence

## Architecture

```
LayerStack
├── Layer 1 (MainLayer) - Base pattern (rainbow, wave players)
├── Layer 2 (PatternLayer) - Pulse mask effect
└── Layer N - Additional effects...

Each Layer has:
├── update(dt) - Update animation state
├── render(output, numLeds) - Render to buffer
└── blender - How to combine with previous layers
```

## LEDBlender Types

### SelectiveMaskBlender
**Purpose**: Creates selective masking effects using intensity values
**Use Case**: Pulse masks, spotlight effects, selective visibility
**How it works**: Uses the red channel of the mask color as an intensity multiplier
```cpp
float maskIntensity = b.r / 255.0f;
return Light(a.r * maskIntensity, a.g * maskIntensity, a.b * maskIntensity);
```

### MultiplyBlender
**Purpose**: Multiplies colors by a blend factor
**Use Case**: Dimming effects, fade transitions
**How it works**: Multiplies each RGB component by the blend factor

### AddBlender
**Purpose**: Adds colors together (with clamping)
**Use Case**: Light accumulation, glow effects, fire
**How it works**: Adds RGB components and clamps to 255

### ScreenBlender
**Purpose**: Soft light blending that preserves highlights
**Use Case**: Soft overlays, light diffusion
**How it works**: Uses screen blend formula: `255 - ((255-a) * (255-b) / 255)`

### HSVContrastBlender
**Purpose**: Creates complementary colors for high contrast
**Use Case**: Text overlays, high-visibility effects
**How it works**: Converts to HSV, shifts hue by 180°, converts back to RGB

## Layer Types

### MainLayer
**Purpose**: Renders the base pattern (wave players, rainbow players)
**Blender**: None (additive by default)
**Output**: Base pattern colors

### PatternLayer
**Purpose**: Applies pulse player masking effects
**Blender**: SelectiveMaskBlender
**Output**: Masked pattern based on pulse player intensity

## LayerStack Rendering Process

1. **Clear Output**: Start with black buffer
2. **For Each Layer**:
   - Call `layer->update(dt)` to update animation state
   - Call `layer->render(tempBuffer, numLeds)` to render layer
   - Get layer's blender: `blender = layer->getBlender()`
   - If blender exists: `output[i] = (*blender)(output[i], tempBuffer[i], 1.0f)`
   - If no blender: Additive blend (default)

## Example: Pulse Mask Effect

```cpp
// Setup
layerStack = std::unique_ptr<LayerStack>(new LayerStack(NUM_LEDS));

// Add main pattern layer
layerStack->addLayer<MainLayer>(&wavePlayer, &rainbow1, &rainbow2);

// Add pulse mask layer
layerStack->addLayer<PatternLayer>(&pulsePlayer, blendBuffer);

// Render loop
layerStack->update(dt);
layerStack->render(leds);
```

**Result**: Moving spotlight effect where pulse player reveals/hides the main pattern

## Adding New Blenders

1. **Define the blender**:
```cpp
struct MyCustomBlender : public LEDBlender {
    Light operator()(const Light& a, const Light& b, float blendFactor) override {
        // Your custom blending logic
        return Light(result_r, result_g, result_b);
    }
};
```

2. **Use in a layer**:
```cpp
class MyLayer : public Layer {
private:
    MyCustomBlender myBlender;
public:
    MyLayer() {
        setBlender(&myBlender);
    }
};
```

## Adding New Layers

1. **Inherit from Layer**:
```cpp
class MyCustomLayer : public Layer {
public:
    void update(float dt) override {
        // Update animation state
    }
    
    void render(Light* output, int numLeds) override {
        // Render to output buffer
    }
};
```

2. **Add to layer stack**:
```cpp
layerStack->addLayer<MyCustomLayer>(args...);
```

## Performance Considerations

- **Memory**: Each layer stack uses one temp buffer
- **CPU**: Rendering cost scales linearly with number of layers
- **Optimization**: Disable unused layers with `setEnabled(false)`

## Best Practices

1. **Layer Order**: Base patterns first, effects on top
2. **Blender Selection**: Choose the right blender for your effect
3. **Memory Management**: Use smart pointers for automatic cleanup
4. **Performance**: Keep layer count reasonable for your target hardware

## Common Patterns

### Spotlight Effect
```cpp
MainLayer + PatternLayer(SelectiveMaskBlender)
```

### Glow Effect
```cpp
MainLayer + GlowLayer(AddBlender)
```

### High Contrast Overlay
```cpp
MainLayer + ContrastLayer(HSVContrastBlender)
```

### Multiple Masks
```cpp
MainLayer + Mask1Layer(SelectiveMaskBlender) + Mask2Layer(SelectiveMaskBlender)
```
