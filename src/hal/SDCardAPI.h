#pragma once
#include "PlatformConfig.h"
#include <Arduino.h>
#include "utility/OutputManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#if SUPPORTS_SD_CARD

class SDCardAPI {
public:
    typedef void (*TaskEnableCallback)();
    
    // Singleton pattern
    static SDCardAPI& getInstance();
    
    // Prevent copying and assignment
    SDCardAPI(const SDCardAPI&) = delete;
    SDCardAPI& operator=(const SDCardAPI&) = delete;
    
    // Initialize the singleton (call once in setup)
    static void initialize(TaskEnableCallback enableCallback = nullptr);
    
    // Thread-safe command handling
    void handleCommand(const String& command);
    void update();
    String getLastResult() const { return lastResult; }
    
    // Set output target for this command
    void setOutputTarget(OutputTarget target);
    OutputTarget getOutputTarget() const;
    
    // Cleanup (call during shutdown)
    static void cleanup();

    // Public facing methods
    uint32_t getFileSize(const String& filename);

private:
    // Private constructor for singleton
    SDCardAPI(TaskEnableCallback enableCallback = nullptr);
    ~SDCardAPI();
    
    // SD card operation mutex
    SemaphoreHandle_t _sdMutex;
    
    TaskEnableCallback enableCallback;
    String lastResult;
    bool busy;
    OutputTarget currentOutputTarget = OutputTarget::BLE; // Default to BLE for backward compatibility
    
    void setResult(const String& result);
    void setError(const String& error);
    
    void printFile(const String& filename);
    void listFiles(const String& dir, int levels);
    void deleteFile(const String& filename);
    void writeFile(const String& filename, const String& content);
    void appendFile(const String& filename, const String& content);
    void getFileInfo(const String& filename);
    void moveFile(const String& oldname, const String& newname);
    void copyFile(const String& source, const String& destination);
    void makeDir(const String& dirname);
    void removeDir(const String& dirname);
    void touchFile(const String& filename);
    void renameFile(const String& oldname, const String& newname);
    void existsFile(const String& filename);
    void setErrorJson(const String& command, const String& filename, const String& error);
    
    // Helper for thread-safe SD operations
    bool acquireSDMutex(TickType_t timeout = pdMS_TO_TICKS(1000));
    void releaseSDMutex();
};

#endif // SUPPORTS_SD_CARD 