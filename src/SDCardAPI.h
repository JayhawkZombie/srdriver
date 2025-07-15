#pragma once
#include <Arduino.h>
#include "utility/OutputManager.h"

// Forward declarations
// class FileStreamer;  // Removed - using FreeRTOS BLE task for streaming
class SDCardIndexer;

class SDCardAPI {
public:
    typedef void (*TaskEnableCallback)();
    SDCardAPI(SDCardIndexer& indexer, TaskEnableCallback enableCallback = nullptr);
    void handleCommand(const String& command);
    void update();
    String getLastResult() const { return lastResult; }
    
    // Set output target for this command
    void setOutputTarget(OutputTarget target);
    OutputTarget getOutputTarget() const;
    
private:
    // FileStreamer& fileStreamer;  // Removed - using FreeRTOS BLE task for streaming
    SDCardIndexer& sdIndexer;
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
}; 