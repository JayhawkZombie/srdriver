#include "EffectFactory.h"
#include "WhiteEffect.h"
#include "SolidColorEffect.h"
#include "RainbowEffect.h"
#include "ColorBlendEffect.h"
#include "TwinklingEffect.h"
#include "RainEffect.h"
#include "WavePlayerEffect.h"
#include "PulsePlayerEffect.h"
#include "freertos/LogManager.h"

int EffectFactory::nextEffectId = 1;

void parseColorString(const String& colorString, Light& color)
{
    // Parse RGB color string like "rgb(255,0,0)" or "rgb(0,255,0)"
    if (colorString.startsWith("rgb(") && colorString.endsWith(")"))
    {
        String rgbPart = colorString.substring(4, colorString.length() - 1);
        int firstComma = rgbPart.indexOf(',');
        int secondComma = rgbPart.indexOf(',', firstComma + 1);

        if (firstComma > 0 && secondComma > firstComma)
        {
            int r = rgbPart.substring(0, firstComma).toInt();
            int g = rgbPart.substring(firstComma + 1, secondComma).toInt();
            int b = rgbPart.substring(secondComma + 1).toInt();

            color = Light(r, g, b);
            LOG_DEBUGF_COMPONENT("EffectFactory", "Parsed color rgb(%d,%d,%d)", r, g, b);
        }
        else
        {
            LOG_ERRORF_COMPONENT("EffectFactory", "Invalid RGB format: %s", colorString.c_str());
            color = Light(255, 255, 255); // Default to white
        }
    }
    else
    {
        LOG_ERRORF_COMPONENT("EffectFactory", "Unsupported color format: %s", colorString.c_str());
        color = Light(255, 255, 255); // Default to white
    }
}

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
    else if (effectType == "rainbow") {
        return createRainbowEffect(params);
    }
    else if (effectType == "color_blend") {
        return createColorBlendEffect(params);
    }
    else if (effectType == "twinkle") {
        return createTwinklingEffect(params);
    }
    else if (effectType == "rain") {
        return createRainEffect(params);
    }
    else if (effectType == "wave") {
        return createWavePlayerEffect(params);
    } else if (effectType == "pulse") {
        return createPulsePlayerEffect(params);
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

std::unique_ptr<Effect> EffectFactory::createTwinklingEffect(const JsonObject& params) {
    int numLEDs = 32 * 32;
    int startLED = 0;
    int endLED = numLEDs - 1;

    float starChance = 0.06f;
    float minDuration = 0.01f;
    float maxDuration = 1.0f;
    float minSpawnTime = 0.5f;
    float maxSpawnTime = 1.0f;
    float starBrightness = 0.5f;
    float fadeInSpeed = 1.1f;
    float fadeOutSpeed = 1.1f;


    if (params.containsKey("starChance")) {
        starChance = params["starChance"].as<float>();
    }
    if (params.containsKey("minDuration")) {
        minDuration = params["minDuration"].as<float>();
    }
    if (params.containsKey("maxDuration")) {
        maxDuration = params["maxDuration"].as<float>();
    }
    if (params.containsKey("minSpawnTime")) {
        minSpawnTime = params["minSpawnTime"].as<float>();
    }
    if (params.containsKey("maxSpawnTime")) {
        maxSpawnTime = params["maxSpawnTime"].as<float>();
    }
    if (params.containsKey("maxStarBrightness")) {
        starBrightness = params["maxStarBrightness"].as<float>();
    }
    if (params.containsKey("fadeInSpeed")) {
        fadeInSpeed = params["fadeInSpeed"].as<float>();
    }
    if (params.containsKey("fadeOutSpeed")) {
        fadeOutSpeed = params["fadeOutSpeed"].as<float>();
    }
    // Short forms as well
    if (params.containsKey("mnd")) {
        minDuration = params["mnd"].as<float>();
    }
    if (params.containsKey("mxd")) {
        maxDuration = params["mxd"].as<float>();
    }
    if (params.containsKey("mns")) {
        minSpawnTime = params["mns"].as<float>();
    }
    if (params.containsKey("mxs")) {
        maxSpawnTime = params["mxs"].as<float>();
    }
    if (params.containsKey("sc")) {
        starChance = params["sc"].as<float>();
    }
    if (params.containsKey("mb")) {
        starBrightness = params["mb"].as<float>();
    }
    if (params.containsKey("fis")) {
        fadeInSpeed = params["fis"].as<float>();
    }
    if (params.containsKey("fos")) {
        fadeOutSpeed = params["fos"].as<float>();
    }
    std::unique_ptr<TwinklingEffect> ptr = std::unique_ptr<TwinklingEffect>(new TwinklingEffect(generateEffectId(), numLEDs, startLED, endLED));

    ptr->init();
    ptr->setStarChance(starChance);
    ptr->setDurationRange(minDuration, maxDuration);
    ptr->setSpawnTimeRange(minSpawnTime, maxSpawnTime);
    ptr->setStarBrightness(starBrightness);
    ptr->setFadeSpeeds(fadeInSpeed, fadeOutSpeed);
    return ptr;
}

std::unique_ptr<Effect> EffectFactory::createRainEffect(const JsonObject& params) {
/*
    void setSpawnColumnRange(int minimum, int maximum) { spawnColumnRange = RandomIntInRange(minimum, maximum); }
    void setSpawnRowRange(int minimum, int maximum) { spawnRowRange = RandomIntInRange(minimum, maximum); }
    void setHiLightRange(int minimum, int maximum) { hiLightRange = RandomIntInRange(minimum, maximum); }
    void setLoLightRange(int minimum, int maximum) { loLightRange = RandomIntInRange(minimum, maximum); }
    void setRingWidthRange(float minimum, float maximum) { ringWidthRange = RandomFloatInRange(minimum, maximum); }
    void setLifetimeRange(float minimum, float maximum) { lifetimeRange = RandomFloatInRange(minimum, maximum); }
    void setAmplitudeRange(float minimum, float maximum) { amplitudeRange = RandomFloatInRange(minimum, maximum); }*/
    int spawnColumnRangeMinimum = -8;
    int spawnColumnRangeMaximum = 38;
    int spawnRowRangeMinimum = -8;
    int spawnRowRangeMaximum = 38;
    int hiLightRangeMinimum = 80;
    int hiLightRangeMaximum = 160;
    int loLightRangeMinimum = 16;
    int loLightRangeMaximum = 80;
    float ringWidthRangeMinimum = 1.f;
    float ringWidthRangeMaximum = 8.f;
    float lifetimeRangeMinimum = 0.5f;
    float lifetimeRangeMaximum = 2.0f;
    float amplitudeRangeMinimum = 0.3f;
    float amplitudeRangeMaximum = 1.0f;
    int oddsOfRadiating = 3;
    float speedFactor = 1.0f;
    float spawnTime = 0.5f;
    float tStartFactor = 2.0f;
    int tStartMod = 1000;

    if (params.containsKey("sc_min")) {
        spawnColumnRangeMinimum = params["sc_min"].as<int>();
    }
    if (params.containsKey("sc_max")) {
        spawnColumnRangeMaximum = params["sc_max"].as<int>();
    }
    if (params.containsKey("sr_min")) {
        spawnRowRangeMinimum = params["sr_min"].as<int>();
    }
    if (params.containsKey("sr_max")) {
        spawnRowRangeMaximum = params["sr_max"].as<int>();
    }
    if (params.containsKey("hi_min")) {
        hiLightRangeMinimum = params["hi_min"].as<int>();
    }
    if (params.containsKey("hi_max")) {
        hiLightRangeMaximum = params["hi_max"].as<int>();
    }
    if (params.containsKey("lo_min")) {
        loLightRangeMinimum = params["lo_min"].as<int>();
    }
    if (params.containsKey("lo_max")) {
        loLightRangeMaximum = params["lo_max"].as<int>();
    }
    if (params.containsKey("rw_min")) {
        ringWidthRangeMinimum = params["rw_min"].as<float>();
    }
    if (params.containsKey("rw_max")) {
        ringWidthRangeMaximum = params["rw_max"].as<float>();
    }
    if (params.containsKey("lt_min")) {
        lifetimeRangeMinimum = params["lt_min"].as<float>();
    }
    if (params.containsKey("lt_max")) {
        lifetimeRangeMaximum = params["lt_max"].as<float>();
    }
    if (params.containsKey("amp_min")) {
        amplitudeRangeMinimum = params["amp_min"].as<float>();
    }
    if (params.containsKey("amp_max")) {
        amplitudeRangeMaximum = params["amp_max"].as<float>();
    }
    if (params.containsKey("oor")) {
        oddsOfRadiating = params["oor"].as<int>();
    }
    if (params.containsKey("sf")) {
        speedFactor = params["sf"].as<float>();
    }
    if (params.containsKey("st")) {
        spawnTime = params["st"].as<float>();
    }
    if (params.containsKey("tsf")) {
        tStartFactor = params["tsf"].as<float>();
    }
    if (params.containsKey("tsm")) {
        tStartMod = params["tsm"].as<int>();
    }
    auto ptr = std::unique_ptr<RainEffect>(new RainEffect(generateEffectId()));
    ptr->setSpawnColumnRange(spawnColumnRangeMinimum, spawnColumnRangeMaximum);
    ptr->setSpawnRowRange(spawnRowRangeMinimum, spawnRowRangeMaximum);
    ptr->setHiLightRange(hiLightRangeMinimum, hiLightRangeMaximum);
    ptr->setLoLightRange(loLightRangeMinimum, loLightRangeMaximum);
    ptr->setRingWidthRange(ringWidthRangeMinimum, ringWidthRangeMaximum);
    ptr->setLifetimeRange(lifetimeRangeMinimum, lifetimeRangeMaximum);
    ptr->setAmplitudeRange(amplitudeRangeMinimum, amplitudeRangeMaximum);
    ptr->setOddsOfRadiating(oddsOfRadiating);
    ptr->setSpeedFactor(speedFactor);
    ptr->setSpawnTime(spawnTime);
    ptr->setTStartFactor(tStartFactor);
    ptr->setTStartMod(tStartMod);
    return ptr;
}
    
std::unique_ptr<Effect> EffectFactory::createWavePlayerEffect(const JsonObject& params) {
    WavePlayerConfig wavePlayerConfig;
    /*
                "AmpRt": 0.735,
            "wvLenLt": 41.273,
            "wvLenRt": 14.629,
            "wvSpdLt": 35.004,
            "wvSpdRt": 13.584,
            "C_Rt": [1, 0, 3.478],
            "C_Lt": [0, 0, 0],
            "rightTrigFuncIndex": 0,
            "leftTrigFuncIndex": 0,
            "useRightCoefficients": false,
            "useLeftCoefficients": false,
            "nTermsRt": 0,
            "nTermsLt": 0,
            "speed": 0.03
            */
    wavePlayerConfig.rows = 32;
    wavePlayerConfig.cols = 32;
    wavePlayerConfig.onLight = Light(255, 255, 0);
    wavePlayerConfig.offLight = Light(0, 0, 255);
    wavePlayerConfig.AmpRt = 0.735f;
    wavePlayerConfig.wvLenLt = 41.273f;
    wavePlayerConfig.wvLenRt = 14.629f;
    wavePlayerConfig.wvSpdLt = 35.004f;
    wavePlayerConfig.wvSpdRt = 13.584f;
    wavePlayerConfig.C_Rt[0] = 1.0f;
    wavePlayerConfig.C_Rt[1] = 0.0f;
    wavePlayerConfig.C_Rt[2] = 3.478f;
    wavePlayerConfig.rightTrigFuncIndex = 0;
    wavePlayerConfig.leftTrigFuncIndex = 0;
    wavePlayerConfig.useRightCoefficients = false;
    wavePlayerConfig.useLeftCoefficients = false;
    wavePlayerConfig.nTermsRt = 0;
    wavePlayerConfig.nTermsLt = 0;
    wavePlayerConfig.speed = 1.f;
    String onLightString = "rgb(255,255,255)";
    String offLightString = "rgb(0,0,0)";
    if (params.containsKey("ampRt")) {
        wavePlayerConfig.AmpRt = params["ampRt"].as<float>();
    }
    if (params.containsKey("wvLenLt")) {
        wavePlayerConfig.wvLenLt = params["wvLenLt"].as<float>();
    }
    if (params.containsKey("wvLenRt")) {
        wavePlayerConfig.wvLenRt = params["wvLenRt"].as<float>();
    }
    if (params.containsKey("wvSpdLt")) {
        wavePlayerConfig.wvSpdLt = params["wvSpdLt"].as<float>();
    }
    if (params.containsKey("wvSpdRt")) {
        wavePlayerConfig.wvSpdRt = params["wvSpdRt"].as<float>();
    }
    if (params.containsKey("c_rt") && params["c_rt"].is<JsonArray>()) {
        LOG_DEBUGF_COMPONENT("EffectFactory", "c_rt is a JsonArray");
        wavePlayerConfig.C_Rt[0] = params["c_rt"][0].as<float>();
        wavePlayerConfig.C_Rt[1] = params["c_rt"][1].as<float>();
        wavePlayerConfig.C_Rt[2] = params["c_rt"][2].as<float>();
    }
    if (params.containsKey("c_lt") && params["c_lt"].is<JsonArray>()) {
        LOG_DEBUGF_COMPONENT("EffectFactory", "c_lt is a JsonArray");
        wavePlayerConfig.C_Lt[0] = params["c_lt"][0].as<float>();
        wavePlayerConfig.C_Lt[1] = params["c_lt"][1].as<float>();
        wavePlayerConfig.C_Lt[2] = params["c_lt"][2].as<float>();
    }
    if (params.containsKey("onLight")) {
        onLightString = params["onLight"].as<String>();
    }
    if (params.containsKey("offLight")) {
        offLightString = params["offLight"].as<String>();
    }
    // if (params.containsKey("rtfi")) {
    //     wavePlayerConfig.rightTrigFuncIndex = params["rtfi"].as<int>();
    // }
    // if (params.containsKey("ltfi")) {
    //     wavePlayerConfig.leftTrigFuncIndex = params["ltfi"].as<unsigned int>();
    // }
    if (params.containsKey("urc")) {
        wavePlayerConfig.useRightCoefficients = params["urc"].as<bool>();
    }
    if (params.containsKey("ulc")) {
        wavePlayerConfig.useLeftCoefficients = params["ulc"].as<bool>();
    }
    if (params.containsKey("nTermsRt")) {
        wavePlayerConfig.nTermsRt = params["nTermsRt"].as<unsigned int>();
    }
    if (params.containsKey("nTermsLt")) {
        wavePlayerConfig.nTermsLt = params["nTermsLt"].as<unsigned int>();
    }
    if (params.containsKey("speed")) {
        wavePlayerConfig.speed = params["speed"].as<float>();
    }
    LOG_DEBUGF_COMPONENT("EffectFactory", "onLightString: %s", onLightString.c_str());
    LOG_DEBUGF_COMPONENT("EffectFactory", "offLightString: %s", offLightString.c_str());
    parseColorString(onLightString, wavePlayerConfig.onLight);
    parseColorString(offLightString, wavePlayerConfig.offLight);
    return std::unique_ptr<WavePlayerEffect>(new WavePlayerEffect(generateEffectId(), wavePlayerConfig));
}

std::unique_ptr<Effect> EffectFactory::createPulsePlayerEffect(const JsonObject& params) {
    LOG_DEBUGF_COMPONENT("EffectFactory", "Creating pulse player effect");
    int pulseWidthRangeMinimum = 5;
    int pulseWidthRangeMaximum = 30;
    float pulseSpeedRangeMinimum = 16.0f;
    float pulseSpeedRangeMaximum = 92.0f;
    float pulseTimeBetweenSpawnsRangeMinimum = 0.5f;
    float pulseTimeBetweenSpawnsRangeMaximum = 6.0f;
    bool reverseDirection = false;
    int pulseHiColorHueRangeMinimum = 0;
    int pulseHiColorHueRangeMaximum = 360;
    if (params.containsKey("pw_min")) {
        pulseWidthRangeMinimum = params["pw_min"].as<int>();
    }
    if (params.containsKey("pw_max")) {
        pulseWidthRangeMaximum = params["pw_max"].as<int>();
    }
    if (params.containsKey("ps_min")) {
        pulseSpeedRangeMinimum = params["ps_min"].as<float>();
    }
    if (params.containsKey("ps_max")) {
        pulseSpeedRangeMaximum = params["ps_max"].as<float>();
    }
    if (params.containsKey("tbs_min")) {
        pulseTimeBetweenSpawnsRangeMinimum = params["tbs_min"].as<float>();
    }
    if (params.containsKey("tbs_max")) {
        pulseTimeBetweenSpawnsRangeMaximum = params["tbs_max"].as<float>();
    }
    if (params.containsKey("hi_min")) {
        pulseHiColorHueRangeMinimum = params["hi_min"].as<int>();
    }
    if (params.containsKey("hi_max")) {
        pulseHiColorHueRangeMaximum = params["hi_max"].as<int>();
    }
    if (pulseHiColorHueRangeMinimum > pulseHiColorHueRangeMaximum) {
        int temp = pulseHiColorHueRangeMinimum;
        pulseHiColorHueRangeMinimum = pulseHiColorHueRangeMaximum;
        pulseHiColorHueRangeMaximum = temp;
    }
    LOG_DEBUGF_COMPONENT("EffectFactory", "pulseWidthRangeMinimum: %d, pulseWidthRangeMaximum: %d", pulseWidthRangeMinimum, pulseWidthRangeMaximum);
    LOG_DEBUGF_COMPONENT("EffectFactory", "pulseSpeedRangeMinimum: %f, pulseSpeedRangeMaximum: %f", pulseSpeedRangeMinimum, pulseSpeedRangeMaximum);
    LOG_DEBUGF_COMPONENT("EffectFactory", "pulseTimeBetweenSpawnsRangeMinimum: %f, pulseTimeBetweenSpawnsRangeMaximum: %f", pulseTimeBetweenSpawnsRangeMinimum, pulseTimeBetweenSpawnsRangeMaximum);
    LOG_DEBUGF_COMPONENT("EffectFactory", "pulseHiColorHueRangeMinimum: %d, pulseHiColorHueRangeMaximum: %d", pulseHiColorHueRangeMinimum, pulseHiColorHueRangeMaximum);
    auto ptr = std::unique_ptr<PulsePlayerEffect>(new PulsePlayerEffect(generateEffectId()));
    ptr->setPulseWidthRange(pulseWidthRangeMinimum, pulseWidthRangeMaximum);
    ptr->setPulseSpeedRange(pulseSpeedRangeMinimum, pulseSpeedRangeMaximum);
    ptr->setPulseTimeBetweenSpawnsRange(pulseTimeBetweenSpawnsRangeMinimum, pulseTimeBetweenSpawnsRangeMaximum);
    ptr->setPulseHiColorHueRange(pulseHiColorHueRangeMinimum, pulseHiColorHueRangeMaximum);
    return ptr;
}

int EffectFactory::generateEffectId() {
    return nextEffectId++;
}
