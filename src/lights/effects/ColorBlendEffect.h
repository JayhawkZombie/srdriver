#pragma once

#include "Effect.h"

/**
 * Color blend effect that smoothly transitions between two colors
 * 
 * Creates a flowing blend between color1 and color2 across the LED strip
 */
class ColorBlendEffect : public Effect {
public:
    ColorBlendEffect(int id, const String& color1, const String& color2, float speed = 1.0f, float duration = -1.0f);
    ~ColorBlendEffect() = default;
    
    // Effect interface
    void update(float dt) override;
    void initialize(Light* output, int numLEDs) override;
    void render(Light* output) override;
    bool isFinished() const override;
    
    // Color blend controls
    void setColor1(const String& color);
    void setColor2(const String& color);
    void setSpeed(float speed);
    
private:
    int numLEDs;
    String color1String;
    String color2String;
    Light color1;
    Light color2;
    float speed;
    float duration;
    float elapsed;
    bool hasDuration;
    float blendPosition; // 0.0 to 1.0, cycles between colors
    
    void parseColorString(const String& colorString, Light& color);
    Light blendColors(const Light& c1, const Light& c2, float t);
};
