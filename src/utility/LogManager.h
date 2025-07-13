#pragma once
#include <WString.h>
#include <queue>
#include <mutex>
#include <functional> // Added for std::function

class LogManager {
public:
    enum LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

    // Singleton pattern
    static LogManager& getInstance();
    
    // Simple logging interface
    void log(LogLevel level, const String& message);
    void debug(const String& message);
    void info(const String& message);
    void warn(const String& message);
    void error(const String& message);
    
    // Task interface for processing the queue
    bool hasPendingLogs() const;
    String getNextLog();
    void markLogProcessed();
    
    // Configuration
    void setLogFile(const String& filename);
    void setLogLevel(LogLevel level);
    void setMaxQueueSize(size_t maxSize);
    
    // Get current configuration
    String getLogFile() const { return logFilename; }
    LogLevel getLogLevel() const { return currentLevel; }
    size_t getQueueSize() const;

private:
    LogManager(); // Private constructor for singleton
    
    struct LogEntry {
        unsigned long timestamp;
        LogLevel level;
        String message;
        
        LogEntry(unsigned long ts, LogLevel lvl, const String& msg)
            : timestamp(ts), level(lvl), message(msg) {}
    };
    
    std::queue<LogEntry> logQueue;
    mutable std::mutex queueMutex;
    
    String logFilename;
    LogLevel currentLevel;
    size_t maxQueueSize;
    
    String levelToString(LogLevel level) const;
    String formatLogEntry(const LogEntry& entry) const;
};

// Convenient logging macros
#define LOG_DEBUG(msg) LogManager::getInstance().debug(msg)
#define LOG_INFO(msg) LogManager::getInstance().info(msg)
#define LOG_WARN(msg) LogManager::getInstance().warn(msg)
#define LOG_ERROR(msg) LogManager::getInstance().error(msg) 