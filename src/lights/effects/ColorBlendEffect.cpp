#include "ColorBlendEffect.h"
#include "freertos/LogManager.h"

ColorBlendEffect::ColorBlendEffect(int id, const String& color1, const String& color2, float speed, float duration) 
    : Effect(id), color1String(color1), color2String(color2), speed(speed), duration(duration), elapsed(0.0f) {
    
    hasDuration = (duration > 0.0f);
    blendPosition = 0.0f;
    
    // Parse the color strings
    parseColorString(color1String, this->color1);
    parseColorString(color2String, this->color2);
    
    LOG_DEBUG("ColorBlendEffect: Created with ID " + String(id) + 
              ", color1: " + color1String + 
              ", color2: " + color2String + 
              ", speed: " + String(speed) + 
              ", duration: " + String(duration));
}

void ColorBlendEffect::update(float dt) {
    if (!isActive) return;
    
    elapsed += dt;
    
    // Update blend position based on speed
    blendPosition += speed * dt;
    if (blendPosition > 1.0f) {
        blendPosition = 0.0f; // Reset to start of cycle
    }
    
    // Debug logging every 100 updates
    static int debugCounter = 0;
    if (debugCounter++ % 100 == 0) {
        LOG_DEBUG("ColorBlendEffect: Update - elapsed: " + String(elapsed) + 
                  ", speed: " + String(speed) + 
                  ", blendPosition: " + String(blendPosition));
    }
}

void ColorBlendEffect::render(Light* output, int numLEDs) {
    if (!isActive) return;
    
    // Create a flowing blend across the LED strip
    for (int i = 0; i < numLEDs; i++) {
        // Calculate position along the strip (0.0 to 1.0)
        float stripPosition = (float)i / (float)(numLEDs - 1);
        
        // Add the blend position to create flowing effect
        float blendT = fmod(stripPosition + blendPosition, 1.0f);
        
        // Blend between the two colors
        Light blendedColor = blendColors(color1, color2, blendT);
        output[i] = blendedColor;
    }
}

bool ColorBlendEffect::isFinished() const {
    if (!isActive) return true;
    if (!hasDuration) return false;
    return elapsed >= duration;
}

void ColorBlendEffect::setColor1(const String& color) {
    color1String = color;
    parseColorString(color1String, color1);
}

void ColorBlendEffect::setColor2(const String& color) {
    color2String = color;
    parseColorString(color2String, color2);
}

void ColorBlendEffect::setSpeed(float newSpeed) {
    speed = newSpeed;
}

void ColorBlendEffect::parseColorString(const String& colorString, Light& color) {
    // Parse RGB color string like "rgb(255,0,0)" or "rgb(0,255,0)"
    if (colorString.startsWith("rgb(") && colorString.endsWith(")")) {
        String rgbPart = colorString.substring(4, colorString.length() - 1);
        int firstComma = rgbPart.indexOf(',');
        int secondComma = rgbPart.indexOf(',', firstComma + 1);
        
        if (firstComma > 0 && secondComma > firstComma) {
            int r = rgbPart.substring(0, firstComma).toInt();
            int g = rgbPart.substring(firstComma + 1, secondComma).toInt();
            int b = rgbPart.substring(secondComma + 1).toInt();
            
            color = Light(r, g, b);
            LOG_DEBUG("ColorBlendEffect: Parsed color rgb(" + String(r) + "," + String(g) + "," + String(b) + ")");
        } else {
            LOG_ERROR("ColorBlendEffect: Invalid RGB format: " + colorString);
            color = Light(255, 255, 255); // Default to white
        }
    } else {
        LOG_ERROR("ColorBlendEffect: Unsupported color format: " + colorString);
        color = Light(255, 255, 255); // Default to white
    }
}

Light ColorBlendEffect::blendColors(const Light& c1, const Light& c2, float t) {
    // Simple linear blend between two colors
    // t = 0.0 means all c1, t = 1.0 means all c2
    float r = c1.r + (c2.r - c1.r) * t;
    float g = c1.g + (c2.g - c1.g) * t;
    float b = c1.b + (c2.b - c1.b) * t;
    
    return Light((uint8_t)r, (uint8_t)g, (uint8_t)b);
}
