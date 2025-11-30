#include "UserPreferences.h"
#if SUPPORTS_PREFERENCES

PreferencesManager::PreferencesManager() {}

void PreferencesManager::begin() {
    prefs.begin("userprefs", false); // namespace, read/write
}

void PreferencesManager::load(DeviceState& settings) {
    Serial.println("Loading preferences...");
    Serial.println("Available keys in preferences:");
    // List all available keys (this is a debug feature)
    prefs.begin("userprefs", false); // read/write mode
    Serial.println("Preferences opened for reading");
    
    if (prefs.isKey("brightness")) {
        settings.brightness = prefs.getInt("brightness", settings.brightness);
    }
    if (prefs.isKey("patternIdx")) {
        settings.patternIndex = prefs.getInt("patternIdx", settings.patternIndex);
    }
    if (prefs.isKey("hiR")) {
        settings.highColor.r = prefs.getInt("hiR", settings.highColor.r);
    }
    if (prefs.isKey("hiG")) {
        settings.highColor.g = prefs.getInt("hiG", settings.highColor.g);
    }
    if (prefs.isKey("hiB")) {
        settings.highColor.b = prefs.getInt("hiB", settings.highColor.b);
    }
    if (prefs.isKey("loR")) {
        settings.lowColor.r = prefs.getInt("loR", settings.lowColor.r);
    }
    if (prefs.isKey("loG")) {
        settings.lowColor.g = prefs.getInt("loG", settings.lowColor.g);
    }
    if (prefs.isKey("loB")) {
        settings.lowColor.b = prefs.getInt("loB", settings.lowColor.b);
    }
    if (prefs.isKey("speedMultiplier")) {
        settings.speedMultiplier = prefs.getFloat("speedMultiplier", settings.speedMultiplier);
    }
    if (prefs.isKey("useBackgroundColor")) {
        settings.useBackgroundColor = prefs.getBool("useBackgroundColor", settings.useBackgroundColor);
    }
    if (prefs.isKey("bgR")) {
        settings.backgroundColor.r = prefs.getInt("bgR", settings.backgroundColor.r);
    }
    if (prefs.isKey("bgG")) {
        settings.backgroundColor.g = prefs.getInt("bgG", settings.backgroundColor.g);
    }
    if (prefs.isKey("bgB")) {
        settings.backgroundColor.b = prefs.getInt("bgB", settings.backgroundColor.b);
    }
    if (prefs.isKey("wifiSSID")) {
        settings.wifiSSID = prefs.getString("wifiSSID", settings.wifiSSID);
    }
    if (prefs.isKey("wifiPassword")) {
        settings.wifiPassword = prefs.getString("wifiPassword", settings.wifiPassword);
    }
    Serial.println("Checking for effect_type key...");
    if (prefs.isKey("effect_type")) {
        settings.currentEffectType = prefs.getString("effect_type", settings.currentEffectType);
        Serial.println("Loaded currentEffectType: " + settings.currentEffectType);
    } else {
        Serial.println("effect_type key not found in preferences");
    }
    Serial.println("Checking for effect_params key...");
    if (prefs.isKey("effect_params")) {
        settings.currentEffectParams = prefs.getString("effect_params", settings.currentEffectParams);
        Serial.println("Loaded currentEffectParams: " + settings.currentEffectParams);
    } else {
        Serial.println("effect_params key not found in preferences");
    }
    prefs.end();
}

void PreferencesManager::save(const DeviceState& settings) {
    // Serial.println("Saving user preferences");
    // Serial.println("brightness: " + String(settings.brightness));
    prefs.begin("userprefs", false);
    prefs.putInt("brightness", settings.brightness);
    // Serial.println("patternIndex: " + String(settings.patternIndex));
    prefs.putInt("patternIdx", settings.patternIndex);
    prefs.putInt("hiR", settings.highColor.r);
    prefs.putInt("hiG", settings.highColor.g);
    prefs.putInt("hiB", settings.highColor.b);
    prefs.putInt("loR", settings.lowColor.r);
    prefs.putInt("loG", settings.lowColor.g);
    prefs.putInt("loB", settings.lowColor.b);
    prefs.putFloat("speedMultiplier", settings.speedMultiplier);
    prefs.putBool("useBackgroundColor", settings.useBackgroundColor);
    prefs.putInt("bgR", settings.backgroundColor.r);
    prefs.putInt("bgG", settings.backgroundColor.g);
    prefs.putInt("bgB", settings.backgroundColor.b);
    prefs.putString("wifiSSID", settings.wifiSSID);
    prefs.putString("wifiPassword", settings.wifiPassword);
    // Serial.println("currentEffectType: " + settings.currentEffectType);
    // Serial.println("currentEffectParams: " + settings.currentEffectParams);
    // Serial.println("Saving currentEffectType to preferences..." + settings.currentEffectType);
    prefs.putString("effect_type", settings.currentEffectType);
    // Serial.println("Saving currentEffectParams to preferences..." + settings.currentEffectParams);
    prefs.putString("effect_params", settings.currentEffectParams);
    // Serial.println("Preferences save completed");
    // Add a small delay to ensure preferences are committed to flash
    // delay(100);
    prefs.end();



    prefs.begin("userprefs", false);
    // Check if the preferences are saved
    // Serial.println("Checking if the preferences are saved...");
    // Serial.println("effect_type: " + prefs.getString("effect_type"));
    // Serial.println("effect_params: " + prefs.getString("effect_params"));
    // Serial.println("wifiSSID: " + prefs.getString("wifiSSID"));
    // Serial.println("wifiPassword: " + prefs.getString("wifiPassword"));
    // Serial.println("brightness: " + String(prefs.getInt("brightness")));
    // Serial.println("patternIndex: " + String(prefs.getInt("patternIdx")));
    // Serial.println("hiR: " + String(prefs.getInt("hiR")));
    // Serial.println("hiG: " + String(prefs.getInt("hiG")));
    // Serial.println("hiB: " + String(prefs.getInt("hiB")));
    prefs.end();
}

void PreferencesManager::end() {
    prefs.end();
}
#endif