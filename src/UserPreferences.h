#pragma once
#include "PlatformConfig.h"
#if SUPPORTS_PREFERENCES

#include "../lights/Light.h"


#include <Preferences.h>
#include "DeviceState.h"

class PreferencesManager {
public:
    PreferencesManager();
    void begin();
    void load(DeviceState& settings);
    void save(const DeviceState& settings);
    void end();
private:
    Preferences prefs;
};

#endif
