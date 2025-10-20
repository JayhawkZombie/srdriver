#include "EffectFactory.h"
#include "WhiteEffect.h"
#include "SolidColorEffect.h"
#include "RainbowEffect.h"
#include "ColorBlendEffect.h"
#include "freertos/LogManager.h"

int EffectFactory::nextEffectId = 1;

std::unique_ptr<Effect> EffectFactory::createEffect(const JsonObject& effectCommand) {
    // Support both full and shortened field names
    String effectType = "";
    if (effectCommand.containsKey("type")) {
        effectType = String(effectCommand["type"].as<const char *>());
    } else if (effectCommand.containsKey("t")) {
        effectType = String(effectCommand["t"].as<const char *>());
    }
    
    JsonObject params;
    if (effectCommand.containsKey("parameters")) {
        params = effectCommand["parameters"].as<JsonObject>();
    } else if (effectCommand.containsKey("p")) {
        params = effectCommand["p"].as<JsonObject>();
    }
    
    LOG_DEBUG("EffectFactory: Creating effect of type: " + effectType);
    
    if (effectType == "white") {
        return createWhiteEffect(params);
    }
    else if (effectType == "solid_color") {
        return createSolidColorEffect(params);
    }
    else if (effectType == "wave") {
        return createWaveEffect(params);
    }
    else if (effectType == "rainbow") {
        return createRainbowEffect(params);
    }
    else if (effectType == "color_blend") {
        return createColorBlendEffect(params);
    }
    else {
        LOG_ERROR("EffectFactory: Unknown effect type: " + effectType);
        return nullptr;
    }
}

std::unique_ptr<Effect> EffectFactory::createWhiteEffect(const JsonObject& params) {
    int brightness = 255;
    if (params.containsKey("brightness")) {
        brightness = params["brightness"];
    } else if (params.containsKey("b")) {
        brightness = params["b"];
    }
    
    float duration = -1.0f;
    if (params.containsKey("duration")) {
        duration = params["duration"];
    } else if (params.containsKey("d")) {
        duration = params["d"];
    }
    
    LOG_DEBUG("EffectFactory: Creating white effect - brightness: " + String(brightness) + 
              ", duration: " + String(duration));
    
    return std::unique_ptr<WhiteEffect>(new WhiteEffect(generateEffectId(), brightness, duration));
}

std::unique_ptr<Effect> EffectFactory::createSolidColorEffect(const JsonObject& params) {
    String colorString = "rgb(255,255,255)";
    if (params.containsKey("color")) {
        colorString = String(params["color"].as<const char*>());
    } else if (params.containsKey("c")) {
        colorString = String(params["c"].as<const char*>());
    }
    
    float duration = -1.0f;
    if (params.containsKey("duration")) {
        duration = params["duration"];
    } else if (params.containsKey("d")) {
        duration = params["d"];
    }
    
    LOG_DEBUG("EffectFactory: Creating solid color effect - color: " + colorString + 
              ", duration: " + String(duration));
    
    return std::unique_ptr<SolidColorEffect>(new SolidColorEffect(generateEffectId(), colorString, duration));
}

std::unique_ptr<Effect> EffectFactory::createWaveEffect(const JsonObject& params) {
    // TODO: Implement wave effect when we integrate with existing WavePlayer
    LOG_DEBUG("EffectFactory: Wave effect not yet implemented");
    return nullptr;
}

std::unique_ptr<Effect> EffectFactory::createRainbowEffect(const JsonObject& params) {
    float speed = 1.0f;
    if (params.containsKey("speed")) {
        speed = params["speed"];
    } else if (params.containsKey("s")) {
        speed = params["s"];
    }
    
    bool reverseDirection = false;
    if (params.containsKey("reverse")) {
        reverseDirection = params["reverse"];
    } else if (params.containsKey("r")) {
        reverseDirection = params["r"];
    }
    
    float duration = -1.0f;
    if (params.containsKey("duration")) {
        duration = params["duration"];
    } else if (params.containsKey("d")) {
        duration = params["d"];
    }
    
    LOG_DEBUG("EffectFactory: Creating rainbow effect - speed: " + String(speed) + 
              ", reverse: " + String(reverseDirection) + 
              ", duration: " + String(duration));
    
    return std::unique_ptr<RainbowEffect>(new RainbowEffect(generateEffectId(), speed, reverseDirection, duration));
}

std::unique_ptr<Effect> EffectFactory::createColorBlendEffect(const JsonObject& params) {
    String color1 = "rgb(0,255,0)"; // Default green
    if (params.containsKey("color1")) {
        color1 = params["color1"].as<String>();
    } else if (params.containsKey("c1")) {
        color1 = params["c1"].as<String>();
    }
    
    String color2 = "rgb(0,0,255)"; // Default blue
    if (params.containsKey("color2")) {
        color2 = params["color2"].as<String>();
    } else if (params.containsKey("c2")) {
        color2 = params["c2"].as<String>();
    }
    
    float speed = 1.0f;
    if (params.containsKey("speed")) {
        speed = params["speed"];
    } else if (params.containsKey("s")) {
        speed = params["s"];
    }
    
    float duration = -1.0f;
    if (params.containsKey("duration")) {
        duration = params["duration"];
    } else if (params.containsKey("d")) {
        duration = params["d"];
    }
    
    LOG_DEBUG("EffectFactory: Creating color blend effect - color1: " + color1 + 
              ", color2: " + color2 + 
              ", speed: " + String(speed) + 
              ", duration: " + String(duration));
    
    return std::unique_ptr<ColorBlendEffect>(new ColorBlendEffect(generateEffectId(), color1, color2, speed, duration));
}

int EffectFactory::generateEffectId() {
    return nextEffectId++;
}
