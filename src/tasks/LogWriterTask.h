#pragma once
#include <SD.h>
#include "utility/LogFile.h"

class LogWriterTask {
public:
    LogWriterTask();
    ~LogWriterTask();
    
    // Main update method called by task scheduler
    void update();
    
    // Check if task is active (has pending work)
    bool isActive() const;
    
    // Initialize the task
    void begin();
    
    // Set log file
    void setLogFile(const String& filename);

private:
    bool initialized;
    LogFile* logFile;
    
    // Ensure logs directory exists
    void ensureLogsDirectory();
    
    // Write a single log entry to file
    bool writeLogToFile(const String& logEntry);
}; 