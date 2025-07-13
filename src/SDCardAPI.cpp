#include "SDCardAPI.h"
#include "tasks/FileStreamer.h"
#include "tasks/SDCardIndexer.h"
#include "utility/SDUtils.h"
#include "utility/StringUtils.h"
#include <SD.h>

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
        setError("Another operation is in progress");
        return;
    }
    
    File file = SD.open(filename.c_str());
    if (!file) {
        setError("File not found: " + filename);
        return;
    }
    
    if (file.isDirectory()) {
        file.close();
        setError("Cannot print directory: " + filename);
        return;
    }
    
    // Read the entire file into a string
    String content = "----- FILE CONTENTS BEGIN -----\n";
    content += "File: " + filename + "\n";
    content += "Size: " + String(file.size()) + " bytes\n";
    content += "----- CONTENT START -----\n";
    
    while (file.available()) {
        content += (char)file.read();
    }
    
    content += "\n----- CONTENT END -----\n";
    content += "----- FILE CONTENTS END -----\n";
    
    file.close();
    setResult(content);
}

void SDCardAPI::listFiles(const String& dir, int levels) {
    String result = "----- FILE LISTING BEGIN -----\n";
    result += "Listing directory: " + dir + "\n";
    
    File root = SD.open(dir.c_str());
    if (!root) {
        result += "Failed to open directory\n";
        result += "----- FILE LISTING END -----\n";
        setResult(result);
        return;
    }
    if (!root.isDirectory()) {
        result += "Not a directory\n";
        result += "----- FILE LISTING END -----\n";
        setResult(result);
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            result += "  DIR : " + String(file.name()) + "\n";
            if (levels) {
                // For now, don't recurse to keep it simple
                result += "    [subdirectory contents not shown]\n";
            }
        } else {
            result += "  FILE: " + String(file.name()) + "\tSIZE: " + String(file.size()) + "\n";
        }
        file = root.openNextFile();
    }
    
    result += "----- FILE LISTING END -----\n";
    setResult(result);
}

void SDCardAPI::deleteFile(const String& filename) {
    if (SD.remove(filename.c_str())) {
        setResult("Deleted: " + filename);
    } else {
        setError("Failed to delete: " + filename);
    }
}

void SDCardAPI::writeFile(const String& filename, const String& content) {
    File file = SD.open(filename.c_str(), FILE_WRITE);
    if (file) {
        file.print(content);
        file.close();
        setResult("Written: " + filename);
    } else {
        setError("Failed to write: " + filename);
    }
}

void SDCardAPI::appendFile(const String& filename, const String& content) {
    File file = SD.open(filename.c_str(), FILE_APPEND);
    if (file) {
        file.print(content);
        file.close();
        setResult("Appended: " + filename);
    } else {
        setError("Failed to append: " + filename);
    }
}

void SDCardAPI::getFileInfo(const String& filename) {
    File file = SD.open(filename.c_str());
    if (file) {
        String info = "File: " + filename + ", Size: " + String(file.size()) + " bytes";
        file.close();
        setResult(info);
    } else {
        setError("File not found: " + filename);
    }
}

void SDCardAPI::update() {
    if (busy && !fileStreamer.isActive() && fileStreamer.getBuffer()) {
        Serial.print("----- FILE CONTENTS BEGIN -----");
        Serial.write(fileStreamer.getBuffer(), fileStreamer.getBufferSize());
        Serial.println("\n----- FILE CONTENTS END -----");
        busy = false;
    }
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
    if (SD.rename(oldname.c_str(), newname.c_str())) {
        setResult("Moved: " + oldname + " to " + newname);
    } else {
        setError("Failed to move: " + oldname + " to " + newname);
    }
}

void SDCardAPI::copyFile(const String& source, const String& destination) {
    File srcFile = SD.open(source.c_str(), FILE_READ);
    if (!srcFile) {
        setError("Source file not found: " + source);
        return;
    }
    File destFile = SD.open(destination.c_str(), FILE_WRITE);
    if (!destFile) {
        srcFile.close();
        setError("Failed to open destination: " + destination);
        return;
    }
    uint8_t buf[64];
    size_t n;
    while ((n = srcFile.read(buf, sizeof(buf))) > 0) {
        destFile.write(buf, n);
    }
    srcFile.close();
    destFile.close();
    setResult("Copied: " + source + " to " + destination);
}

void SDCardAPI::makeDir(const String& dirname) {
    if (SD.mkdir(dirname.c_str())) {
        setResult("Directory created: " + dirname);
    } else {
        setError("Failed to create directory: " + dirname);
    }
}

void SDCardAPI::removeDir(const String& dirname) {
    if (SD.rmdir(dirname.c_str())) {
        setResult("Directory removed: " + dirname);
    } else {
        setError("Failed to remove directory: " + dirname);
    }
}

void SDCardAPI::touchFile(const String& filename) {
    File file = SD.open(filename.c_str(), FILE_WRITE);
    if (file) {
        file.close();
        setResult("Touched file: " + filename);
    } else {
        setError("Failed to touch file: " + filename);
    }
}

void SDCardAPI::renameFile(const String& oldname, const String& newname) {
    if (SD.rename(oldname.c_str(), newname.c_str())) {
        setResult("Renamed: " + oldname + " to " + newname);
    } else {
        setError("Failed to rename: " + oldname + " to " + newname);
    }
}

void SDCardAPI::existsFile(const String& filename) {
    if (SD.exists(filename.c_str())) {
        setResult("Exists: " + filename);
    } else {
        setResult("Does not exist: " + filename);
    }
} 