#include "PlatformConfig.h"
#include "hal/PlatformFactory.h"
#include "hal/SDCardController.h"
#include "freertos/LogManager.h"

void testPlatformAbstractions() {
    // Test platform detection
    LOG_PRINTF("Platform: %s", PlatformFactory::getPlatformName());
    
    // Test feature support
    LOG_PRINTF("BLE Support: %s", PlatformFactory::supportsBLE() ? "YES" : "NO");
    LOG_PRINTF("WiFi Support: %s", PlatformFactory::supportsWiFi() ? "YES" : "NO");
    LOG_PRINTF("Display Support: %s", PlatformFactory::supportsDisplay() ? "YES" : "NO");
    LOG_PRINTF("SD Card Support: %s", PlatformFactory::supportsSDCard() ? "YES" : "NO");
    LOG_PRINTF("Preferences Support: %s", PlatformFactory::supportsPreferences() ? "YES" : "NO");
    LOG_PRINTF("ESP32 APIs Support: %s", PlatformFactory::supportsESP32APIs() ? "YES" : "NO");
    
    // Test system information
    LOG_PRINTF("Free Heap: %d bytes", PlatformFactory::getFreeHeap());
    LOG_PRINTF("Total Heap: %d bytes", PlatformFactory::getHeapSize());
    LOG_PRINTF("CPU Frequency: %d MHz", PlatformFactory::getCpuFreqMHz());
    LOG_PRINTF("Min Free Heap: %d bytes", PlatformFactory::getMinFreeHeap());
    
    // Test SD card HAL
    SDCardController* sdController = PlatformFactory::createSDCardController();
    if (sdController) {
        LOG_PRINTF("SD Card Controller created successfully");
        LOG_PRINTF("SD Card Available: %s", sdController->isAvailable() ? "YES" : "NO");
        
        // Test file operations (should fail gracefully if not available)
        bool writeResult = sdController->writeFile("/test.txt", "Hello Platform HAL!");
        LOG_PRINTF("Write Test Result: %s", writeResult ? "SUCCESS" : "FAILED (expected)");
        
        String readResult = sdController->readFile("/test.txt");
        LOG_PRINTF("Read Test Result: %s", readResult.length() > 0 ? "SUCCESS" : "FAILED (expected)");
        
        delete sdController;
    } else {
        LOG_ERROR("Failed to create SD Card Controller");
    }
    
    LOG_INFO("Platform abstraction test completed");
} 