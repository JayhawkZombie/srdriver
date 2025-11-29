#include "InputDeviceConfig.h"
#include "config/JsonSettings.h"
#include "../SDCardController.h"
#include "freertos/LogManager.h"
#include <ArduinoJson.h>

DynamicJsonDocument jsonHardwareBuilderDoc(8196);

HardwareInputTaskBuilder& HardwareInputTaskBuilder::fromJson(const String& jsonFileName) {
    if (g_sdCardController->isAvailable()) {
        String jsonString = g_sdCardController->readFile(jsonFileName.c_str());
        DeserializationError error = deserializeJson(jsonHardwareBuilderDoc, jsonString);
        if (error) {
            LOG_ERRORF("Failed to deserialize JSON: %s", error.c_str());
            return *this;
        }
    }
    if (jsonHardwareBuilderDoc.containsKey("inputDevices")) {
        LOG_INFO("Loading input devices from JSON");
        JsonArray devices = jsonHardwareBuilderDoc["inputDevices"];
        for (int i = 0; i < devices.size(); i++) {
            LOG_INFOF("Loading device %d", i);
            const JsonObject &device = devices[i];
            String name = device["name"] | "";
            LOG_INFOF("Device name: %s", name.c_str());
            String type = device["type"] | "";
            LOG_INFOF("Device type: %s", type.c_str());
            String pinName = device["pin"] | "oops";
            LOG_INFOF("Device pin name: %s", pinName.c_str());
            int pin = pinNameToNumber(pinName); // D0 default?
            LOG_INFOF("Device pin: %d", pin);
            uint32_t pollInterval = device["pollIntervalMs"] | 50;
            LOG_INFOF("Device poll interval: %d", pollInterval);

            if (name.length() > 0 && pin >= 0) {
                InputDeviceConfig config(name, type, pin, pollInterval);
                // Load device-specific parameters
                if (type == "microphone") {
                    LOG_INFO("Loading microphone device");
                    config.sampleRate = device["sampleRate"] | 8000;
                    config.sampleWindow = device["sampleWindow"] | 50;
                } else if (type == "potentiometer") {
                    config.hysteresisThreshold = device["hysteresisThreshold"] | 50;
                }
                configs.push_back(config);
            }
        }
    }
    return *this;
}
