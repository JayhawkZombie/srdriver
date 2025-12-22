#include "PlatformFactory.h"
#include "PlatformConfig.h"

#if PLATFORM_ESP32_S3
#include "esp32/ESP32PlatformFactory.h"
#endif

#if PLATFORM_CROW_PANEL
#include "esp32/CrowPanelPlatformFactory.h"
#endif

SDCardController *PlatformFactory::createSDCardController()
{

#if SUPPORTS_SD_CARD

    #if PLATFORM_CROW_PANEL
    return new CrowPanelSDCardController();
    #elif PLATFORM_ESP32_S3
    return new ESP32SDCardController();
    #else
    return new NullSDCardController();
    #endif
#else
    return nullptr;
#endif
}

DS18B20Component* PlatformFactory::createTemperatureSensor(int pin)
{
#if SUPPORTS_TEMPERATURE_SENSOR
    return new DS18B20Component(pin);
#else
    return nullptr;
#endif
}
