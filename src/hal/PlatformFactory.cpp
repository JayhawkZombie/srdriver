#include "PlatformFactory.h"
#include "PlatformConfig.h"

#if PLATFORM_ESP32_S3
#include "esp32/ESP32PlatformFactory.h"
#endif

SDCardController *PlatformFactory::createSDCardController()
{
#if SUPPORTS_SD_CARD
    return new ESP32SDCardController();
#else
    return new NullSDCardController();
#endif
}
