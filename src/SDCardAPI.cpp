#include "SDCardAPI.h"
#include "tasks/FileStreamer.h"
#include "tasks/SDCardIndexer.h"
#include "utility/SDUtils.h"
#include "utility/StringUtils.h"
#include <SD.h>
#include <ArduinoJson.h>
#include <base64.h>
#include "BLEManager.h"
#include "tasks/JsonChunkStreamer.h"
extern BLEManager bleManager;

// Add a static buffer for base64 content (for now, not thread-safe)
static String printBase64Buffer;

SDCardAPI::SDCardAPI(FileStreamer& streamer, SDCardIndexer& indexer, TaskEnableCallback enableCallback)
    : fileStreamer(streamer), sdIndexer(indexer), enableCallback(enableCallback), busy(false) {}

void SDCardAPI::handleCommand(const String& command) {
    auto tokens = splitString(command, ' ');
    if (tokens.empty()) return;
    String cmd = tokens[0];
    cmd.toUpperCase();
    String dir = "/";
    int levels = 0;
    if (cmd == "LIST") {
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
    String arg1 = tokens.size() > 1 ? tokens[1] : "";
    String arg2 = tokens.size() > 2 ? tokens[2] : "";

    if (cmd == "PRINT") {
        printFile(arg1);
    } else if (cmd == "DELETE") {
        deleteFile(arg1);
    } else if (cmd == "WRITE") {
        if (arg1.length() > 0) {
            auto [filename, content] = splitFirst(arg1, ':');
            writeFile(filename, content);
        } else {
            setError("WRITE command requires filename:content");
        }
    } else if (cmd == "APPEND") {
        if (arg1.length() > 0) {
            auto [filename, content] = splitFirst(arg1, ':');
            appendFile(filename, content);
        } else {
            setError("APPEND command requires filename:content");
        }
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
    if (busy) {
        setErrorJson("PRINT", filename, "Another operation is in progress");
        return;
    }
    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) {
        setErrorJson("PRINT", filename, "File not found or busy");
        return;
    }
    printBase64Buffer = "";
    const size_t bufSize = 512;
    uint8_t buf[bufSize];
    size_t n;
    while ((n = file.read(buf, bufSize)) > 0) {
        printBase64Buffer += base64EncodeBuffer(buf, n);
    }
    file.close();
    busy = true;
    // Start streaming the base64-encoded file content as a single JSON string
    DynamicJsonDocument doc(600);
    doc["c"] = "PRINT";
    doc["p"] = printBase64Buffer;
    String fullJson;
    serializeJson(doc, fullJson);
    bleManager.startStreaming(fullJson, "PRINT");
    busy = false;
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
    setResult(result);
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
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to write";
    }
    String result;
    serializeJson(doc, result);
    setResult(result);
}

void SDCardAPI::appendFile(const String& filename, const String& content) {
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
    } else {
        doc["ok"] = 0;
        doc["err"] = "Failed to append";
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
    Serial.println("API Result: " + result);
}

void SDCardAPI::setError(const String& error) {
    lastResult = "ERROR: " + error;
    Serial.println("API Error: " + error);
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