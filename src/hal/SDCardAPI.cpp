#include "SDCardAPI.h"
#include "PlatformConfig.h"
#if SUPPORTS_SD_CARD

#include "PlatformConfig.h"
#include "hal/SDCardController.h"
#include "utility/SDUtils.h"
#include "utility/StringUtils.h"
#include "utility/OutputManager.h"
#include "freertos/LogManager.h"
#include <ArduinoJson.h>
#include <base64.h>
#include "hal/ble/BLEManager.h"
#include "tasks/JsonChunkStreamer.h"
// Remove the extern declaration since we're using singleton
// extern BLEManager bleManager;

// Add a static buffer for base64 content (for now, not thread-safe)
static String printBase64Buffer;

// Singleton instance
static SDCardAPI* g_instance = nullptr;

// Singleton implementation
SDCardAPI& SDCardAPI::getInstance() {
    if (!g_instance) {
        LOG_ERROR("SDCardAPI not initialized! Call SDCardAPI::initialize() first");
        // Return a reference to a static instance to prevent crashes
        static SDCardAPI fallbackInstance;
        return fallbackInstance;
    }
    return *g_instance;
}

void SDCardAPI::initialize(TaskEnableCallback enableCallback) {
    if (g_instance) {
        LOG_WARN("SDCardAPI already initialized");
        return;
    }
    g_instance = new SDCardAPI(enableCallback);
    LOG_INFO("SDCardAPI singleton initialized");
}

void SDCardAPI::cleanup() {
    if (g_instance) {
        delete g_instance;
        g_instance = nullptr;
        LOG_INFO("SDCardAPI singleton cleaned up");
    }
}

// Thread safety helpers
bool SDCardAPI::acquireSDMutex(TickType_t timeout) {
    if (_sdMutex == nullptr) {
        LOG_ERROR("SD mutex not initialized");
        return false;
    }
    
    BaseType_t result = xSemaphoreTake(_sdMutex, timeout);
    if (result == pdTRUE) {
        return true;
    } else {
        LOG_WARN("Failed to acquire SD mutex (timeout)");
        return false;
    }
}

void SDCardAPI::releaseSDMutex() {
    if (_sdMutex != nullptr) {
        xSemaphoreGive(_sdMutex);
    }
}

SDCardAPI::SDCardAPI(TaskEnableCallback enableCallback)
    : enableCallback(enableCallback), busy(false) {
    // Create mutex for SD card operations
    _sdMutex = xSemaphoreCreateMutex();
    if (_sdMutex == nullptr) {
        LOG_ERROR("Failed to create SD mutex");
    } else {
        LOG_INFO("SD mutex created successfully");
    }
}

SDCardAPI::~SDCardAPI() {
    if (_sdMutex != nullptr) {
        vSemaphoreDelete(_sdMutex);
        _sdMutex = nullptr;
        LOG_INFO("SD mutex deleted");
    }
}

void SDCardAPI::setOutputTarget(OutputTarget target) {
    currentOutputTarget = target;
}

OutputTarget SDCardAPI::getOutputTarget() const {
    return currentOutputTarget;
}

void SDCardAPI::handleCommand(const String& command) {
    auto tokens = splitString(command, ' ');
    if (tokens.empty()) return;
    String cmd = tokens[0];
    cmd.toUpperCase();
    
    LOG_INFO("Processing command: " + cmd);
    
    // Special handling for commands that need to preserve spaces in content
    if (cmd == "WRITE" || cmd == "APPEND") {
        if (tokens.size() < 2) {
            setError(cmd + " command requires filename:content");
            return;
        }
        
        // Find the colon in the second token to split filename and content
        String arg = tokens[1];
        int colonIndex = arg.indexOf(':');
        if (colonIndex == -1) {
            setError(cmd + " command requires filename:content format");
            return;
        }
        
        String filename = arg.substring(0, colonIndex);
        String content = arg.substring(colonIndex + 1);
        
        // If there are more tokens, append them to content (preserving spaces)
        for (size_t i = 2; i < tokens.size(); i++) {
            content += " " + tokens[i];
        }
        
        if (cmd == "WRITE") {
            writeFile(filename, content);
        } else { // APPEND
            appendFile(filename, content);
        }
        return;
    }
    
    // Handle LIST command
    if (cmd == "LIST") {
        String dir = "/";
        int levels = 0;
        if (tokens.size() == 2) {
            if (tokens[1] == "*") {
                levels = -1;
            } else if (tokens[1][0] == '/') {
                dir = tokens[1];
            } else {
                levels = tokens[1].toInt();
            }
        } else if (tokens.size() >= 3) {
            dir = tokens[1];
            if (tokens[2] == "*") levels = -1;
            else levels = tokens[2].toInt();
        }
        listFiles(dir, levels);
        return;
    }
    
    // Handle other commands
    String arg1 = tokens.size() > 1 ? tokens[1] : "";
    String arg2 = tokens.size() > 2 ? tokens[2] : "";

    if (cmd == "PRINT") {
        printFile(arg1);
    } else if (cmd == "DELETE") {
        deleteFile(arg1);
    } else if (cmd == "INFO") {
        getFileInfo(arg1);
    } else if (cmd == "MOVE") {
        auto [oldname, newname] = splitFirst(arg1, ':');
        moveFile(oldname, newname);
    } else if (cmd == "COPY") {
        auto [source, destination] = splitFirst(arg1, ':');
        copyFile(source, destination);
    } else if (cmd == "MKDIR") {
        makeDir(arg1);
    } else if (cmd == "RMDIR") {
        removeDir(arg1);
    } else if (cmd == "TOUCH") {
        touchFile(arg1);
    } else if (cmd == "RENAME") {
        auto [oldname, newname] = splitFirst(arg1, ':');
        renameFile(oldname, newname);
    } else if (cmd == "EXISTS") {
        existsFile(arg1);
    } else if (cmd == "ROTATE") {
        // Manually trigger log rotation
        LogManager::getInstance().rotateLogs();
        DynamicJsonDocument doc(256);
        doc["ok"] = 1;
        doc["msg"] = "rotation completed";
        doc["ts"] = millis() / 1000;
        String result;
        serializeJson(doc, result);
        setResult(result);
    } else if (cmd == "LOG_FILTER_WIFI_ONLY") {
        // Show only WiFiManager logs
        std::vector<String> wifiOnly = {"WiFiManager"};
        LogManager::getInstance().setComponentFilter(wifiOnly);
        DynamicJsonDocument doc(256);
        doc["ok"] = 1;
        doc["msg"] = "Log filter set to WiFiManager only";
        doc["ts"] = millis() / 1000;
        String result;
        serializeJson(doc, result);
        setResult(result);
    } else if (cmd == "LOG_FILTER_NETWORK") {
        // Show WiFiManager and BLEManager logs
        std::vector<String> networkComponents = {"WiFiManager", "BLEManager"};
        LogManager::getInstance().setComponentFilter(networkComponents);
        DynamicJsonDocument doc(256);
        doc["ok"] = 1;
        doc["msg"] = "Log filter set to network components";
        doc["ts"] = millis() / 1000;
        String result;
        serializeJson(doc, result);
        setResult(result);
    } else if (cmd == "LOG_FILTER_ALL") {
        // Show all logs (disable filtering)
        LogManager::getInstance().enableAllComponents();
        LogManager::getInstance().disableTimestampFilter();
        DynamicJsonDocument doc(256);
        doc["ok"] = 1;
        doc["msg"] = "Log filtering disabled - showing all logs";
        doc["ts"] = millis() / 1000;
        String result;
        serializeJson(doc, result);
        setResult(result);
    } else if (cmd == "LOG_FILTER_NEW_ONLY") {
        // Show only new logs (filter out old ones)
        LogManager::getInstance().setNewLogsOnly();
        DynamicJsonDocument doc(256);
        doc["ok"] = 1;
        doc["msg"] = "Log filter set to new logs only";
        doc["ts"] = millis() / 1000;
        String result;
        serializeJson(doc, result);
        setResult(result);
    } else if (cmd.startsWith("LOG_ADD_COMPONENT:")) {
        // Add component to filter: LOG_ADD_COMPONENT:PatternManager
        String component = cmd.substring(18); // Remove "LOG_ADD_COMPONENT:"
        LogManager::getInstance().addComponent(component);
        DynamicJsonDocument doc(256);
        doc["ok"] = 1;
        doc["msg"] = "Added component to filter: " + component;
        doc["ts"] = millis() / 1000;
        String result;
        serializeJson(doc, result);
        setResult(result);
    } else if (cmd.startsWith("LOG_REMOVE_COMPONENT:")) {
        // Remove component from filter: LOG_REMOVE_COMPONENT:BLEManager
        String component = cmd.substring(21); // Remove "LOG_REMOVE_COMPONENT:"
        LogManager::getInstance().removeComponent(component);
        DynamicJsonDocument doc(256);
        doc["ok"] = 1;
        doc["msg"] = "Removed component from filter: " + component;
        doc["ts"] = millis() / 1000;
        String result;
        serializeJson(doc, result);
        setResult(result);
    } else if (cmd == "LOG_STATUS") {
        // Show current filtering status
        DynamicJsonDocument doc(512);
        doc["ok"] = 1;
        doc["componentFiltering"] = LogManager::getInstance().isComponentFilteringEnabled();
        doc["timestampFiltering"] = LogManager::getInstance().isTimestampFilteringEnabled();
        doc["minTimestamp"] = LogManager::getInstance().getMinTimestamp();
        
        JsonArray allowedComponents = doc.createNestedArray("allowedComponents");
        auto components = LogManager::getInstance().getAllowedComponents();
        for (const String& comp : components) {
            allowedComponents.add(comp);
        }
        
        doc["ts"] = millis() / 1000;
        String result;
        serializeJson(doc, result);
        setResult(result);
    } else if (cmd == "ARCHIVES") {
        listFiles("/logs/archives", 1);
    } else {
        setError("Unknown command: '" + cmd + "'");
    }
}

void SDCardAPI::printFile(const String& filename) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    SDCardFileHandle* handle = g_sdCardController->open(filename.c_str(), "r");
    if (!handle) {     // Send error envelope
        DynamicJsonDocument doc(128);
        doc["t"] = "D";
        doc["s"] = 1;
        doc["n"] = 1;
        doc["p"] = "";
        doc["e"] = true;
        doc["f"] = filename;
        doc["b"] = true;
        doc["err"] = "Could not open file";
        String envelope;
        serializeJson(doc, envelope);
        BLEManager::getInstance()->sendFileDataChunk(envelope); // Send directly
        Serial.print("[SDCardAPI] Failed to open file for PRINT: ");
        Serial.println(filename);
        return;
    }
    
    const size_t chunkSize = 64; // Reduced chunk size for BLE safety
    uint8_t buf[chunkSize];
    size_t n;
    size_t chunkIdx = 1;
    size_t fileSize = g_sdCardController->size(handle);
    size_t totalChunks = (fileSize + chunkSize - 1) / chunkSize;
    if (totalChunks == 0) totalChunks = 1;
    
    while ((n = g_sdCardController->read(handle, buf, chunkSize)) > 0) {
        String b64 = base64EncodeBuffer(buf, n);
        DynamicJsonDocument doc(256 + b64.length());
        doc["t"] = "D";
        doc["s"] = chunkIdx;
        doc["n"] = totalChunks;
        doc["p"] = b64;
        doc["e"] = (chunkIdx == totalChunks);
        doc["f"] = filename;
        doc["b"] = true;
        String envelope;
        serializeJson(doc, envelope);
        BLEManager::getInstance()->sendFileDataChunk(envelope); // Send directly
        chunkIdx++;
        delay(10); // Give BLE stack a chance to breathe
    }
    
    g_sdCardController->close(handle);
}

void SDCardAPI::listFiles(const String& dir, int levels) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    DynamicJsonDocument doc(4096);
    doc["ok"] = 1;
    doc["c"] = "LIST";
    doc["d"] = dir;
    doc["t"] = "d";
    JsonArray children = doc.createNestedArray("ch");
    
    // Use the platform abstractions listDir method
    bool success = g_sdCardController->listDir(dir.c_str(), [&children, &dir](const char* filename, bool isDirectory, size_t size) {
        JsonObject child = children.createNestedObject();
        child["f"] = String(filename);
        
        // Set the correct type based on whether it's a directory or file
        child["t"] = isDirectory ? "d" : "f";
        
        // Get file size for files (not directories)
        if (!isDirectory) {
            // Construct the full path by combining directory with filename
            String fullPath = dir;
            if (dir.endsWith("/")) {
                fullPath += filename;
            } else {
                fullPath += "/" + String(filename);
            }
            
            // Open the file temporarily to get its size
            child["sz"] = size;
        } else {
            child["sz"] = 0; // Directories don't have a size
        }
        
        child["ts"] = 0; // Timestamp not available
    });
    
    if (!success) {
        doc["ok"] = 0;
        doc["err"] = "Failed to list directory";
    }
    
    uint32_t ts = 0;
    doc["ts"] = ts;
    String result;
    serializeJson(doc, result);
    
    // Use OutputManager to route the output appropriately
    OutputManager& outputManager = OutputManager::getInstance();
    outputManager.setOutputTarget(currentOutputTarget);
    
    if (currentOutputTarget == OutputTarget::BLE) {
        // For BLE, stream the JSON
        outputManager.streamToBLE(result, "FILE_LIST");
    } else {
        // For serial, show a readable directory listing
        outputManager.printDirectoryListing(dir, result);
    }
    
    // Don't call setResult here since we've already handled the output
    // setResult(result);
}

void SDCardAPI::deleteFile(const String& filename) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    bool ok = g_sdCardController->remove(filename.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "DELETE";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (ok) {
        doc["ok"] = 1;
        doc["msg"] = "Deleted";
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to delete";
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::writeFile(const String& filename, const String& content) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    // Ensure the directory exists
    String dir = filename.substring(0, filename.lastIndexOf('/'));
    if (dir.length() > 0 && !g_sdCardController->exists(dir.c_str())) {
        g_sdCardController->mkdir(dir.c_str());
        LOG_INFO("Created directory: " + dir);
    }
    
    bool success = g_sdCardController->writeFile(filename.c_str(), content.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "WRITE";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (success) {
        doc["ok"] = 1;
        doc["msg"] = "Written";
        LOG_INFO("File written successfully: " + filename + " (" + String(content.length()) + " bytes)");
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to write - directory doesn't exist or SD card error";
        LOG_ERROR("Failed to write file: " + filename);
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::appendFile(const String& filename, const String& content) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    // Ensure the directory exists
    String dir = filename.substring(0, filename.lastIndexOf('/'));
    if (dir.length() > 0 && !g_sdCardController->exists(dir.c_str())) {
        g_sdCardController->mkdir(dir.c_str());
        LOG_INFO("Created directory: " + dir);
    }
    
    bool success = g_sdCardController->appendFile(filename.c_str(), content.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "APPEND";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (success) {
        doc["ok"] = 1;
        doc["msg"] = "Appended";
        LOG_INFO("Content appended to file: " + filename + " (" + String(content.length()) + " bytes)");
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to append - file not found or directory doesn't exist";
        LOG_ERROR("Failed to append to file: " + filename);
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::getFileInfo(const String& filename) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // First check if it's a directory by trying to open it as a directory
    SDCardFileHandle* dirHandle = g_sdCardController->openDir(filename.c_str());
    if (dirHandle) {
        // It's a directory
        g_sdCardController->close(dirHandle);
        DynamicJsonDocument doc(256);
        doc["c"] = "INFO";
        doc["f"] = filename;
        doc["ok"] = 1;
        doc["t"] = "d";
        doc["sz"] = 0; // Directories don't have a size
        doc["ts"] = 0; // Timestamp not available in platform abstraction
        String result;
        serializeJson(doc, result);
        setResult(result);
        return;
    }
    
    // Try to open as a file
    SDCardFileHandle* handle = g_sdCardController->open(filename.c_str(), "r");
    DynamicJsonDocument doc(256);
    doc["c"] = "INFO";
    doc["f"] = filename;
    if (handle) {
        doc["ok"] = 1;
        doc["sz"] = g_sdCardController->size(handle);
        doc["t"] = "f"; // Assume file for now
        doc["ts"] = 0; // Timestamp not available in platform abstraction
        g_sdCardController->close(handle);
    } else {
        doc["ok"] = 0;
        doc["err"] = "File not found";
        doc["ts"] = millis() / 1000;
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
}

void SDCardAPI::update() {
    // No per-chunk streaming needed for PRINT anymore
    // LIST and PRINT both use jsonStreamer in BLEManager
}

void SDCardAPI::setResult(const String& result) {
    lastResult = result;
    
    // Use OutputManager to route the output appropriately
    OutputManager& outputManager = OutputManager::getInstance();
    outputManager.setOutputTarget(currentOutputTarget);
    
    if (currentOutputTarget == OutputTarget::SERIAL_OUTPUT) {
        // For serial, parse JSON and show user-friendly messages
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, result);
        if (!error) {
            String command = doc["c"] | "";
            String filename = doc["f"] | "";
            String message = doc["msg"] | "";
            String errorMsg = doc["err"] | "";
            // Robust: treat ok:1 as success
            bool success = doc["ok"].as<int>() == 1;
            
            if (command == "LIST") {
                // LIST is handled separately in listFiles method
                return;
            } else if (command == "INFO") {
                if (success) {
                    String type = doc["t"] | "";
                    long size = doc["sz"] | 0;
                    String typeStr = (type == "d") ? "Directory" : "File";
                    outputManager.println(typeStr + ": " + filename);
                    if (type == "f") {
                        outputManager.println("Size: " + String(size) + " bytes");
                    }
                } else {
                    outputManager.println("Error: " + errorMsg);
                }
            } else if (command == "EXISTS") {
                bool exists = doc["ex"] | false;
                outputManager.println("File '" + filename + "' " + (exists ? "exists" : "does not exist"));
            } else {
                // For other commands, show success/error message
                if (success) {
                    outputManager.println("✓ " + message + ": " + filename);
                } else {
                    outputManager.println("✗ Error: " + errorMsg + " (" + filename + ")");
                }
            }
        } else {
            // Fallback to JSON if parsing fails
            outputManager.printJson(result);
        }
    } else {
        // For BLE, just log to serial for debugging
        Serial.println("API Result: " + result);
    }
}

void SDCardAPI::setError(const String& error) {
    lastResult = "ERROR: " + error;
    
    // Use OutputManager to route the output appropriately
    OutputManager& outputManager = OutputManager::getInstance();
    outputManager.setOutputTarget(currentOutputTarget);
    
    if (currentOutputTarget == OutputTarget::SERIAL_OUTPUT) {
        // For serial, print the error directly
        outputManager.println("Error: " + error);
    } else {
        // For BLE, just log to serial for debugging
        Serial.println("API Error: " + error);
    }
}

void SDCardAPI::moveFile(const String& oldname, const String& newname) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    bool ok = g_sdCardController->rename(oldname.c_str(), newname.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "MOVE";
    doc["fr"] = oldname;
    doc["to"] = newname;
    doc["ts"] = millis() / 1000;
    if (ok) {
        doc["ok"] = 1;
        doc["msg"] = "Moved";
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to move";
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::copyFile(const String& source, const String& destination) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    // Read source file using platform abstraction
    String sourceContent = g_sdCardController->readFile(source.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "COPY";
    doc["fr"] = source;
    doc["to"] = destination;
    doc["ts"] = millis() / 1000;
    if (sourceContent.length() == 0) {
        doc["ok"] = 0;
        doc["err"] = "Source file not found";
        String result;
        serializeJson(doc, result);
        setResult(result);
        releaseSDMutex();
        return;
    }
    
    // Write to destination using platform abstraction
    bool success = g_sdCardController->writeFile(destination.c_str(), sourceContent.c_str());
    if (success) {
        doc["ok"] = 1;
        doc["msg"] = "Copied";
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to write destination";
    }
    
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::makeDir(const String& dirname) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    bool ok = g_sdCardController->mkdir(dirname.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "MKDIR";
    doc["f"] = dirname;
    doc["ts"] = millis() / 1000;
    if (ok) {
        doc["ok"] = 1;
        doc["msg"] = "Directory created";
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to create directory";
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::removeDir(const String& dirname) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    // Note: Platform abstraction doesn't have rmdir, so well use remove
    bool ok = g_sdCardController->remove(dirname.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "RMDIR";
    doc["f"] = dirname;
    doc["ts"] = millis() / 1000;
    if (ok) {
        doc["ok"] = 1;
        doc["msg"] = "Directory removed";
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to remove directory";
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::touchFile(const String& filename) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    // Touch by writing empty content
    bool success = g_sdCardController->writeFile(filename.c_str(), "");
    DynamicJsonDocument doc(256);
    doc["c"] = "TOUCH";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (success) {
        doc["ok"] = 1;
        doc["msg"] = "Touched";
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to touch file";
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::renameFile(const String& oldname, const String& newname) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    // Acquire SD mutex for thread safety
    if (!acquireSDMutex()) {
        setError("SD card busy - operation timed out");
        return;
    }
    
    bool ok = g_sdCardController->rename(oldname.c_str(), newname.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "RENAME";
    doc["fr"] = oldname;
    doc["to"] = newname;
    doc["ts"] = millis() / 1000;
    if (ok) {
        doc["ok"] = 1;
        doc["msg"] = "Renamed";
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to rename";
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
    
    // Release SD mutex
    releaseSDMutex();
}

void SDCardAPI::existsFile(const String& filename) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    
    DynamicJsonDocument doc(128);
    doc["c"] = "EXISTS";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (g_sdCardController->exists(filename.c_str())) {
        doc["ok"] = 1;
        doc["ex"] = 1;
    } else {
        doc["ok"] = 1;
        doc["ex"] = 0;
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
}

void SDCardAPI::setErrorJson(const String& command, const String& filename, const String& error) {
    DynamicJsonDocument doc(128);
    doc["ok"] = 0;
    doc["c"] = command;
    doc["f"] = filename;
    doc["err"] = error;
    doc["ts"] = millis() / 1000;
    String result;
    serializeJson(doc, result);
    setResult(result);
} 

uint32_t SDCardAPI::getFileSize(const String& filename) {
    // Get the global SD card controller
    extern SDCardController* g_sdCardController;
    SDCardFileHandle *handle = g_sdCardController->open(filename.c_str(), "r");
    if (handle) {
        uint32_t size = g_sdCardController->size(handle);
        g_sdCardController->close(handle);
        return size;
    }
    return 0;
}

#endif // SUPPORTS_SD_CARD 