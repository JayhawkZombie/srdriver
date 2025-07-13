#pragma once
#include <SD.h>
#include <WString.h>
#include "LogManager.h"

class LogFile {
public:
    enum class FlushMode {
        IMMEDIATE,    // Write each line immediately
        BUFFERED,     // Buffer and flush periodically
        AUTO_FLUSH    // Flush when buffer reaches threshold
    };

    LogFile(const String& filename, FlushMode mode = FlushMode::AUTO_FLUSH);
    ~LogFile();
    
    // File management
    bool open();
    void close();
    bool isOpen() const { return fileOpen; }
    
    // Simple logging
    void log(LogManager::LogLevel level, const String& message);
    void logLine(const String& line);
    
    // Multi-line logging
    void beginEntry(LogManager::LogLevel level, const String& message);
    void addLine(const String& line);
    void endEntry();
    
    // Buffer management
    void flush();
    void setBufferSize(size_t size);
    void setAutoFlushThreshold(size_t threshold);
    
    // Utility
    size_t getFileSize() const;
    String getFilename() const { return filename; }

private:
    String filename;
    File file;
    String buffer;
    size_t bufferSize;
    size_t autoFlushThreshold;
    FlushMode flushMode;
    bool fileOpen;
    bool inMultiLineEntry;
    unsigned long entryStartTime;
    LogManager::LogLevel currentLevel;
    
    // Helper methods
    void writeToFile(const String& data);
    void writeTimestamp();
    String levelToString(LogManager::LogLevel level) const;
    void checkAutoFlush();
}; 