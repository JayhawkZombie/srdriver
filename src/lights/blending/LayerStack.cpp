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

template<typename T, typename... Args>
void LayerStack::addLayer(Args&&... args) {
    layers.push_back(std::unique_ptr<T>(new T(std::forward<Args>(args)...)));
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
            // Simple additive blend for now
            for (int i = 0; i < numLeds; i++) {
                output[i].r = std::min(255, output[i].r + tempBuffer[i].r);
                output[i].g = std::min(255, output[i].g + tempBuffer[i].g);
                output[i].b = std::min(255, output[i].b + tempBuffer[i].b);
            }
        }
    }
}
