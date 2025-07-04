#include "UserPreferences.h"

PreferencesManager::PreferencesManager() {}

void PreferencesManager::begin() {
    prefs.begin("userprefs", false); // namespace, read/write
}

void PreferencesManager::load(UserPreferences& settings) {
    settings.brightness = prefs.getInt("brightness", settings.brightness);
    settings.patternIndex = prefs.getInt("patternIdx", settings.patternIndex);
    settings.highColor.r = prefs.getInt("hiR", settings.highColor.r);
    settings.highColor.g = prefs.getInt("hiG", settings.highColor.g);
    settings.highColor.b = prefs.getInt("hiB", settings.highColor.b);
    settings.lowColor.r = prefs.getInt("loR", settings.lowColor.r);
    settings.lowColor.g = prefs.getInt("loG", settings.lowColor.g);
    settings.lowColor.b = prefs.getInt("loB", settings.lowColor.b);
}

void PreferencesManager::save(const UserPreferences& settings) {
    prefs.putInt("brightness", settings.brightness);
    prefs.putInt("patternIdx", settings.patternIndex);
    prefs.putInt("hiR", settings.highColor.r);
    prefs.putInt("hiG", settings.highColor.g);
    prefs.putInt("hiB", settings.highColor.b);
    prefs.putInt("loR", settings.lowColor.r);
    prefs.putInt("loG", settings.lowColor.g);
    prefs.putInt("loB", settings.lowColor.b);
}

void PreferencesManager::end() {
    prefs.end();
}
