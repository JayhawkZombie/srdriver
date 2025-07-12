#include "SDCardIndexer.h"

SDCardIndexer::DirState::DirState(String p, uint8_t l) : path(p), levels(l), dir() {
}

SDCardIndexer::SDCardIndexer() : active(false), finished(false), levels(0), fileCount(0) {
}

void SDCardIndexer::begin(const char* rootDir, uint8_t maxLevels) {
    dirStack.clear();
    dirStack.push_back({String(rootDir), maxLevels});
    active = true;
    finished = false;
    fileCount = 0;
}

void SDCardIndexer::update() {
    if (!active || dirStack.empty()) {
        active = false;
        finished = true;
        Serial.print("SDCard indexing DONE. Files indexed: ");
        Serial.println(fileCount);
        return;
    }
    
    DirState& current = dirStack.back();
    if (!current.dir) {
        current.dir = SD.open(current.path.c_str());
        if (!current.dir) {
            Serial.print("Failed to open directory: ");
            Serial.println(current.path);
            dirStack.pop_back();
            return;
        }
        if (!current.dir.isDirectory()) {
            Serial.print("Not a directory: ");
            Serial.println(current.path);
            current.dir.close();
            dirStack.pop_back();
            return;
        }
    }
    
    File entry = current.dir.openNextFile();
    if (entry) {
        if (fileCount >= MAX_FILES) {
            Serial.println("WARNING: File index cap reached, some files not indexed!");
            entry.close();
            current.dir.close();
            dirStack.clear();
            active = false;
            finished = true;
            return;
        }
        
        if (entry.isDirectory()) {
            fileList[fileCount++] = {String(entry.name()), true, 0};
            if (current.levels > 0) {
                dirStack.push_back({String(entry.name()), static_cast<uint8_t>(current.levels - 1)});
            }
        } else {
            fileList[fileCount++] = {String(entry.name()), false, (size_t)entry.size()};
        }
        entry.close();
    } else {
        current.dir.close();
        dirStack.pop_back();
    }
}

bool SDCardIndexer::isActive() const { 
    return active; 
}

bool SDCardIndexer::isFinished() const { 
    return finished; 
}

size_t SDCardIndexer::getFileCount() const { 
    return fileCount; 
}

const SDCardIndexer::FileEntry& SDCardIndexer::getFile(size_t idx) const { 
    return fileList[idx]; 
} 