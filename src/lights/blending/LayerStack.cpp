#include "LayerStack.h"
#include "Layer.h"
#include <algorithm>
#include <memory>

LayerStack::LayerStack(int ledCount) : numLeds(ledCount) {
    tempBuffer = new Light[ledCount];
}

LayerStack::~LayerStack() {
    delete[] tempBuffer;
}

void LayerStack::update(float dt) {
    for (auto& layer : layers) {
        if (layer->isEnabled()) {
            layer->update(dt);
        }
    }
}

void LayerStack::render(Light* output) {
    // Clear output buffer
    for (int i = 0; i < numLeds; i++) {
        output[i] = Light(0, 0, 0);
    }

    // Render each layer in order
    for (auto& layer : layers) {
        if (layer->isEnabled()) {
            layer->render(tempBuffer, numLeds);
            
            // Get the layer's blender
            LEDBlender* blender = layer->getBlender();
            
            if (blender) {
                // Use the layer's blender to combine with current output
                for (int i = 0; i < numLeds; i++) {
                    output[i] = (*blender)(output[i], tempBuffer[i], 1.0f);
                }
            } else {
                // No blender specified, use additive as default
                for (int i = 0; i < numLeds; i++) {
                    output[i].r = std::min(255, output[i].r + tempBuffer[i].r);
                    output[i].g = std::min(255, output[i].g + tempBuffer[i].g);
                    output[i].b = std::min(255, output[i].b + tempBuffer[i].b);
                }
            }
        }
    }
}
