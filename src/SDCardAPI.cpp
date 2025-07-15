#include "SDCardAPI.h"
// #include "tasks/FileStreamer.h"  // Removed - using FreeRTOS BLE task for streaming
#include "tasks/SDCardIndexer.h"
#include "utility/SDUtils.h"
#include "utility/StringUtils.h"
#include "utility/OutputManager.h"
#include "utility/LogManager.h"
#include <SD.h>
#include <ArduinoJson.h>
#include <base64.h>
#include "BLEManager.h"
#include "tasks/JsonChunkStreamer.h"
extern BLEManager bleManager;

// Add a static buffer for base64 content (for now, not thread-safe)
static String printBase64Buffer;

SDCardAPI::SDCardAPI(SDCardIndexer& indexer, TaskEnableCallback enableCallback)
    : sdIndexer(indexer), enableCallback(enableCallback), busy(false) {}

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
    } else {
        setError("Unknown command: '" + cmd + "'");
    }
}

void SDCardAPI::printFile(const String& filename) {
    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) {
        // Send error envelope
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
        bleManager.sendFileDataChunk(envelope); // Send directly
        Serial.print("[SDCardAPI] Failed to open file for PRINT: ");
        Serial.println(filename);
        return;
    }
    const size_t chunkSize = 64; // Reduced chunk size for BLE safety
    uint8_t buf[chunkSize];
    size_t n;
    size_t chunkIdx = 1;
    size_t fileSize = file.size();
    size_t totalChunks = (fileSize + chunkSize - 1) / chunkSize;
    if (totalChunks == 0) totalChunks = 1;
    while ((n = file.read(buf, chunkSize)) > 0) {
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
        bleManager.sendFileDataChunk(envelope); // Send directly
        chunkIdx++;
        delay(10); // Give BLE stack a chance to breathe
    }
    file.close();
}

void SDCardAPI::listFiles(const String& dir, int levels) {
    DynamicJsonDocument doc(4096);
    doc["ok"] = 1;
    doc["c"] = "LIST";
    doc["d"] = dir;
    doc["t"] = "d";
    JsonArray children = doc.createNestedArray("ch");
    File rootDir = SD.open(dir.c_str());
    uint32_t ts = 0;
#ifdef FILE_WRITE
    ts = rootDir.getLastWrite();
#endif
    if (!rootDir) {
        setErrorJson("LIST", dir, "Failed to open directory");
        return;
    }
    if (!rootDir.isDirectory()) {
        setErrorJson("LIST", dir, "Not a directory");
        rootDir.close();
        return;
    }
    File file = rootDir.openNextFile();
    while (file) {
        JsonObject child = children.createNestedObject();
        child["f"] = String(file.name());
        child["t"] = file.isDirectory() ? "d" : "f";
        if (!file.isDirectory()) {
            child["sz"] = file.size();
        }
#ifdef FILE_WRITE
        child["ts"] = file.getLastWrite();
#else
        child["ts"] = 0;
#endif
        file.close();
        file = rootDir.openNextFile();
    }
    rootDir.close();
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
    bool ok = SD.remove(filename.c_str());
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
}

void SDCardAPI::writeFile(const String& filename, const String& content) {
    // Ensure the directory exists
    String dir = filename.substring(0, filename.lastIndexOf('/'));
    if (dir.length() > 0 && !SD.exists(dir.c_str())) {
        SD.mkdir(dir.c_str());
        LOG_INFO("Created directory: " + dir);
    }
    
    File file = SD.open(filename.c_str(), FILE_WRITE);
    DynamicJsonDocument doc(256);
    doc["c"] = "WRITE";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (file) {
        file.print(content);
        file.close();
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
}

void SDCardAPI::appendFile(const String& filename, const String& content) {
    // Ensure the directory exists
    String dir = filename.substring(0, filename.lastIndexOf('/'));
    if (dir.length() > 0 && !SD.exists(dir.c_str())) {
        SD.mkdir(dir.c_str());
        LOG_INFO("Created directory: " + dir);
    }
    
    File file = SD.open(filename.c_str(), FILE_APPEND);
    DynamicJsonDocument doc(256);
    doc["c"] = "APPEND";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (file) {
        file.print(content);
        file.close();
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
}

void SDCardAPI::getFileInfo(const String& filename) {
    File file = SD.open(filename.c_str());
    DynamicJsonDocument doc(256);
    doc["c"] = "INFO";
    doc["f"] = filename;
    if (file) {
        doc["ok"] = 1;
        doc["sz"] = file.size();
        doc["t"] = file.isDirectory() ? "d" : "f";
#ifdef FILE_WRITE
        doc["ts"] = file.getLastWrite();
#else
        doc["ts"] = 0;
#endif
        file.close();
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
    bool ok = SD.rename(oldname.c_str(), newname.c_str());
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
}

void SDCardAPI::copyFile(const String& source, const String& destination) {
    File srcFile = SD.open(source.c_str(), FILE_READ);
    DynamicJsonDocument doc(256);
    doc["c"] = "COPY";
    doc["fr"] = source;
    doc["to"] = destination;
    doc["ts"] = millis() / 1000;
    if (!srcFile) {
        doc["ok"] = 0;
        doc["err"] = "Source file not found";
        String result;
        serializeJson(doc, result);
        setResult(result);
        return;
    }
    File destFile = SD.open(destination.c_str(), FILE_WRITE);
    if (!destFile) {
        srcFile.close();
        doc["ok"] = 0;
        doc["err"] = "Failed to open destination";
        String result;
        serializeJson(doc, result);
        setResult(result);
        return;
    }
    uint8_t buf[64];
    size_t n;
    while ((n = srcFile.read(buf, sizeof(buf))) > 0) {
        destFile.write(buf, n);
    }
    srcFile.close();
    destFile.close();
    doc["ok"] = 1;
    doc["msg"] = "Copied";
    String result;
    serializeJson(doc, result);
    setResult(result);
}

void SDCardAPI::makeDir(const String& dirname) {
    bool ok = SD.mkdir(dirname.c_str());
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
}

void SDCardAPI::removeDir(const String& dirname) {
    bool ok = SD.rmdir(dirname.c_str());
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
}

void SDCardAPI::touchFile(const String& filename) {
    File file = SD.open(filename.c_str(), FILE_WRITE);
    DynamicJsonDocument doc(256);
    doc["c"] = "TOUCH";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (file) {
        file.close();
        doc["ok"] = 1;
        doc["msg"] = "Touched";
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to touch file";
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
}

void SDCardAPI::renameFile(const String& oldname, const String& newname) {
    bool ok = SD.rename(oldname.c_str(), newname.c_str());
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
}

void SDCardAPI::existsFile(const String& filename) {
    DynamicJsonDocument doc(128);
    doc["c"] = "EXISTS";
    doc["f"] = filename;
    doc["ts"] = millis() / 1000;
    if (SD.exists(filename.c_str())) {
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