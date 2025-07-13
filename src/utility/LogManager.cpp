#include "LogManager.h"
#include <SD.h>

LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

LogManager::LogManager() 
    : logFilename("/logs/app.log")
    , currentLevel(INFO)
    , maxQueueSize(100) {
}

void LogManager::log(LogLevel level, const String& message) {
    if (level < currentLevel) return; // Skip if below current log level
    
    std::lock_guard<std::mutex> lock(queueMutex);
    
    // Check if queue is full
    if (logQueue.size() >= maxQueueSize) {
        // Remove oldest entry to make room
        logQueue.pop();
    }
    
    // Add new log entry
    logQueue.emplace(millis(), level, message);
    
    // Debug: print when log is added
    Serial.print("[LogManager] Log queued (level=");
    Serial.print(levelToString(level));
    Serial.print(", queue size=");
    Serial.print(logQueue.size());
    Serial.print("): ");
    Serial.println(message);
}

void LogManager::debug(const String& message) {
    log(DEBUG, message);
}

void LogManager::info(const String& message) {
    log(INFO, message);
}

void LogManager::warn(const String& message) {
    log(WARN, message);
}

void LogManager::error(const String& message) {
    log(ERROR, message);
}

bool LogManager::hasPendingLogs() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return !logQueue.empty();
}

String LogManager::getNextLog() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (logQueue.empty()) {
        return "";
    }
    
    const LogEntry& entry = logQueue.front();
    return formatLogEntry(entry);
}

void LogManager::markLogProcessed() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!logQueue.empty()) {
        logQueue.pop();
    }
}

void LogManager::setLogFile(const String& filename) {
    logFilename = filename;
}

void LogManager::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void LogManager::setMaxQueueSize(size_t maxSize) {
    maxQueueSize = maxSize;
}

void LogManager::rotateLogFile() {
    // Check if SD card is available first
    if (!SD.exists("/")) {
        Serial.println("[LogManager] No SD card available - skipping log rotation");
        return;
    }
    
    // Check if the current log file exists
    if (!SD.exists(logFilename.c_str())) {
        Serial.println("[LogManager] No existing log file to rotate");
        return;
    }
    
    // Create timestamp for the archived filename
    // Format: /logs/srdriver_YYYYMMDD_HHMMSS.log
    unsigned long now = millis();
    String timestamp = String(now / 1000); // Simple timestamp based on seconds since boot
    
    // Extract the base filename and extension
    int lastSlash = logFilename.lastIndexOf('/');
    int lastDot = logFilename.lastIndexOf('.');
    
    String basePath = logFilename.substring(0, lastSlash + 1); // Include the slash
    String baseName = (lastDot > lastSlash) ? 
        logFilename.substring(lastSlash + 1, lastDot) : 
        logFilename.substring(lastSlash + 1);
    String extension = (lastDot > lastSlash) ? logFilename.substring(lastDot) : "";
    
    // Create the archived filename
    String archivedFilename = basePath + baseName + "_" + timestamp + extension;
    
    // Rename the current log file to the archived name
    if (SD.rename(logFilename.c_str(), archivedFilename.c_str())) {
        Serial.print("[LogManager] Log file rotated: ");
        Serial.print(logFilename);
        Serial.print(" -> ");
        Serial.println(archivedFilename);
    } else {
        Serial.println("[LogManager] Failed to rotate log file");
    }
}

size_t LogManager::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return logQueue.size();
}

String LogManager::levelToString(LogLevel level) const {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO";
        case WARN:  return "WARN";
        case ERROR: return "ERROR";
        default:    return "UNKNOWN";
    }
}

String LogManager::formatLogEntry(const LogEntry& entry) const {
    // Format: [timestamp] LEVEL: message
    return "[" + String(entry.timestamp) + "] " + levelToString(entry.level) + ": " + entry.message;
} 