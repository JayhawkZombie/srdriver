#include "UserPreferences.h"

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
    prefs.end();
}

void PreferencesManager::end() {
    prefs.end();
}
