#pragma once
#include "Light.h"


#include <Preferences.h>

struct UserPreferences {
    int brightness = 128;
    int patternIndex = 0;
    Light highColor = Light(255, 255, 255);
    Light lowColor = Light(0, 0, 0);
};

class PreferencesManager {
public:
    PreferencesManager();
    void begin();
    void load(UserPreferences& settings);
    void save(const UserPreferences& settings);
    void end();
private:
    Preferences prefs;
};


