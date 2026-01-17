#include "SolidColorEffect.h"
#include "freertos/LogManager.h"

SolidColorEffect::SolidColorEffect(int id, const String& colorString, float duration) 
    : Effect(id), duration(duration), elapsed(0.0f) {
    
    hasDuration = (duration > 0.0f);
    parseColorString(colorString);
    
    LOG_DEBUG("SolidColorEffect: Created with ID " + String(id) + 
              ", color: " + colorString + 
              ", duration: " + String(duration));
}

void SolidColorEffect::update(float dt) {
    if (!isActive) return;
    
    elapsed += dt;
    
    // Debug logging every 100 updates
    static int debugCounter = 0;
    if (debugCounter++ % 100 == 0) {
        LOG_DEBUG("SolidColorEffect: Update - elapsed: " + String(elapsed) + 
                  ", color: rgb(" + String(red) + "," + String(green) + "," + String(blue) + ")");
    }
}

void SolidColorEffect::initialize(Light* output, int numLEDs) {
    this->numLEDs = numLEDs;
    (void)output; // Not needed for simple effects
    LOG_DEBUGF_COMPONENT("SolidColorEffect", "Initialized with %d LEDs", numLEDs);
}

void SolidColorEffect::render(Light* output) {
    if (!isActive) return;
    
    // Render solid color LEDs (brightness is controlled by FastLED.setBrightness globally)
    for (int i = 0; i < numLEDs; i++) {
        output[i] = Light(red, green, blue);
    }
}

bool SolidColorEffect::isFinished() const {
    if (!isActive) return true;
    if (!hasDuration) return false;
    return elapsed >= duration;
}

void SolidColorEffect::parseColorString(const String& colorString) {
    // Parse "rgb(r,g,b)" format
    if (colorString.startsWith("rgb(") && colorString.endsWith(")")) {
        String values = colorString.substring(4, colorString.length() - 1);
        
        int firstComma = values.indexOf(',');
        int secondComma = values.indexOf(',', firstComma + 1);
        
        if (firstComma > 0 && secondComma > firstComma) {
            red = values.substring(0, firstComma).toInt();
            green = values.substring(firstComma + 1, secondComma).toInt();
            blue = values.substring(secondComma + 1).toInt();
            
            // Clamp values to valid range
            red = constrain(red, 0, 255);
            green = constrain(green, 0, 255);
            blue = constrain(blue, 0, 255);
            
            LOG_DEBUG("SolidColorEffect: Parsed color rgb(" + String(red) + "," + String(green) + "," + String(blue) + ")");
            return;
        }
    }
    
    // Fallback to white if parsing fails
    red = 255;
    green = 255;
    blue = 255;
    LOG_WARN("SolidColorEffect: Failed to parse color string: " + colorString + ", using white");
}
