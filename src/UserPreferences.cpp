#include "UserPreferences.h"
#if SUPPORTS_PREFERENCES

PreferencesManager::PreferencesManager() {}

void PreferencesManager::begin() {
    prefs.begin("userprefs", false); // namespace, read/write
}

void PreferencesManager::load(DeviceState& settings) {
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
}

void PreferencesManager::save(const DeviceState& settings) {
    Serial.println("Saving user preferences");
    Serial.println("brightness: " + String(settings.brightness));
    prefs.begin("userprefs", false);
    prefs.putInt("brightness", settings.brightness);
    Serial.println("patternIndex: " + String(settings.patternIndex));
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
    prefs.end();
}

void PreferencesManager::end() {
    prefs.end();
}
#endif