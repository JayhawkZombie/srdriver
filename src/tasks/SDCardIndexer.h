#pragma once
#include <SD.h>
#include <vector>
#include <WString.h>

class SDCardIndexer {
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
    
    SDCardIndexer();
    
    void begin(const char* rootDir, uint8_t maxLevels);
    void update();
    
    bool isActive() const;
    bool isFinished() const;
    size_t getFileCount() const;
    const FileEntry& getFile(size_t idx) const;
    
private:
    bool active;
    bool finished;
    uint8_t levels;
    size_t fileCount;
    FileEntry fileList[MAX_FILES];
    std::vector<DirState> dirStack;
}; 