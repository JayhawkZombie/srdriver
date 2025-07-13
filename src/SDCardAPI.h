#pragma once
#include <Arduino.h>

// Forward declarations
class FileStreamer;
class SDCardIndexer;

class SDCardAPI {
public:
    typedef void (*TaskEnableCallback)();
    SDCardAPI(FileStreamer& streamer, SDCardIndexer& indexer, TaskEnableCallback enableCallback = nullptr);
    void handleCommand(const String& command);
    void update();
    String getLastResult() const { return lastResult; }
private:
    FileStreamer& fileStreamer;
    SDCardIndexer& sdIndexer;
    TaskEnableCallback enableCallback;
    String lastResult;
    bool busy;
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
}; 