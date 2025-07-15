#pragma once

#include "SRTask.h"
#include "LogManager.h"
#include <SD.h>
#include <vector>
#include <WString.h>

/**
 * SDCardIndexerTask - FreeRTOS task for SD card file indexing
 * 
 * Handles:
 * - Background SD card file system indexing
 * - Non-blocking file discovery
 * - File list management for SD card operations
 */
class SDCardIndexerTask : public SRTask {
public:
    struct FileEntry {
        String path;
        bool isDir;
        size_t size;
    };
    
    struct DirState {
        String path;
        uint8_t levels;
        File dir;
        DirState(String p, uint8_t l);
    };
    
    static const size_t MAX_FILES = 100;
    
    SDCardIndexerTask(uint32_t updateIntervalMs = 1,  // 1ms for fast indexing
                      uint32_t stackSize = 8192,
                      UBaseType_t priority = tskIDLE_PRIORITY + 1,
                      BaseType_t core = 0)  // Pin to core 0 (WiFi/BLE core)
        : SRTask("SDIndexer", stackSize, priority, core),
          _updateIntervalMs(updateIntervalMs),
          _active(false),
          _finished(false),
          _levels(0),
          _fileCount(0) {}
    
    /**
     * Start indexing a directory
     */
    void begin(const char* rootDir, uint8_t maxLevels) {
        _dirStack.clear();
        _dirStack.push_back({String(rootDir), maxLevels});
        _active = true;
        _finished = false;
        _fileCount = 0;
        LOG_INFOF("Started SD card indexing: %s (max levels: %d)", rootDir, maxLevels);
    }
    
    /**
     * Check if indexing is active
     */
    bool isActive() const { return _active; }
    
    /**
     * Check if indexing is finished
     */
    bool isFinished() const { return _finished; }
    
    /**
     * Get number of files indexed
     */
    size_t getFileCount() const { return _fileCount; }
    
    /**
     * Get file entry by index
     */
    const FileEntry& getFile(size_t idx) const { 
        if (idx < _fileCount) {
            return _fileList[idx];
        }
        // Return empty entry if index out of bounds
        static const FileEntry emptyEntry = {"", false, 0};
        return emptyEntry;
    }
    
    /**
     * Get update interval
     */
    uint32_t getUpdateInterval() const { return _updateIntervalMs; }
    
    /**
     * Get frame count (number of update cycles)
     */
    uint32_t getFrameCount() const { return _frameCount; }

protected:
    void run() override {
        LOG_INFO("SDCardIndexerTask started");
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        
        while (true) {
            // Update indexing if active
            if (_active) {
                updateIndexing();
            }
            
            _frameCount++;
            
            // Sleep until next cycle
            SRTask::sleepUntil(&lastWakeTime, _updateIntervalMs);
        }
    }

private:
    void updateIndexing() {
        if (!_active || _dirStack.empty()) {
            _active = false;
            _finished = true;
            LOG_INFOF("SD card indexing complete. Files indexed: %d", _fileCount);
            return;
        }
        
        DirState& current = _dirStack.back();
        if (!current.dir) {
            current.dir = SD.open(current.path.c_str());
            if (!current.dir) {
                LOG_WARNF("Failed to open directory: %s", current.path.c_str());
                _dirStack.pop_back();
                return;
            }
            if (!current.dir.isDirectory()) {
                LOG_WARNF("Not a directory: %s", current.path.c_str());
                current.dir.close();
                _dirStack.pop_back();
                return;
            }
        }
        
        File entry = current.dir.openNextFile();
        if (entry) {
            if (_fileCount >= MAX_FILES) {
                LOG_WARN("File index cap reached, some files not indexed!");
                entry.close();
                current.dir.close();
                _dirStack.clear();
                _active = false;
                _finished = true;
                return;
            }
            
            if (entry.isDirectory()) {
                _fileList[_fileCount++] = {String(entry.name()), true, 0};
                if (current.levels > 0) {
                    _dirStack.push_back({String(entry.name()), static_cast<uint8_t>(current.levels - 1)});
                }
            } else {
                _fileList[_fileCount++] = {String(entry.name()), false, (size_t)entry.size()};
            }
            entry.close();
        } else {
            current.dir.close();
            _dirStack.pop_back();
        }
    }
    
    uint32_t _updateIntervalMs;
    bool _active;
    bool _finished;
    uint8_t _levels;
    size_t _fileCount;
    FileEntry _fileList[MAX_FILES];
    std::vector<DirState> _dirStack;
    uint32_t _frameCount = 0;
};

// Constructor for DirState
inline SDCardIndexerTask::DirState::DirState(String p, uint8_t l) : path(p), levels(l), dir() {
} 