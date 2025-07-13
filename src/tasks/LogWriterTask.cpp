#include "LogWriterTask.h"
#include "utility/LogManager.h"

LogWriterTask::LogWriterTask() 
    : initialized(false)
    , logFile(nullptr) {
}

LogWriterTask::~LogWriterTask() {
    if (logFile) {
        logFile->close();
        delete logFile;
        logFile = nullptr;
    }
}

void LogWriterTask::begin() {
    Serial.println("[LogWriterTask] Beginning initialization");
    ensureLogsDirectory();
    Serial.println("[LogWriterTask] Logs directory ensured");
    
    // Create log file with auto-flush mode
    logFile = new LogFile("/logs/srdriver.log", LogFile::FlushMode::AUTO_FLUSH);
    if (logFile->open()) {
        Serial.println("[LogWriterTask] Log file opened successfully");
    } else {
        Serial.println("[LogWriterTask] Failed to open log file");
    }
    
    initialized = true;
    Serial.println("[LogWriterTask] Initialization complete");
}

void LogWriterTask::setLogFile(const String& filename) {
    if (logFile) {
        logFile->close();
        delete logFile;
    }
    
    logFile = new LogFile(filename, LogFile::FlushMode::AUTO_FLUSH);
    if (logFile->open()) {
        Serial.print("[LogWriterTask] Switched to log file: ");
        Serial.println(filename);
    } else {
        Serial.print("[LogWriterTask] Failed to open log file: ");
        Serial.println(filename);
    }
}

void LogWriterTask::update() {
    if (!initialized || !logFile) {
        return;
    }
    
    LogManager& logManager = LogManager::getInstance();
    
    // Check if we have pending logs
    if (logManager.hasPendingLogs()) {
        // Debug: print queue size
        Serial.print("[LogWriterTask] Processing logs, queue size: ");
        Serial.println(logManager.getQueueSize());
        
        // Process one log entry per update cycle to avoid blocking
        String logEntry = logManager.getNextLog();
        if (logEntry.length() > 0) {
            Serial.print("[LogWriterTask] Writing log entry: ");
            Serial.println(logEntry);
            
            if (writeLogToFile(logEntry)) {
                logManager.markLogProcessed();
                Serial.println("[LogWriterTask] Log entry written successfully");
            } else {
                Serial.println("[LogWriterTask] Failed to write log entry");
                // If write failed, keep the entry in queue for retry
                // But don't block forever - could add retry count later
            }
        }
    } else {
        // No pending logs, just return (task stays enabled)
        // Serial.println("[LogWriterTask] No pending logs, continuing to run");
    }
}

bool LogWriterTask::isActive() const {
    return initialized; // Task is always active once initialized
}

void LogWriterTask::ensureLogsDirectory() {
    Serial.print("[LogWriterTask] Checking if /logs directory exists... ");
    if (!SD.exists("/logs")) {
        Serial.println("Creating /logs directory");
        if (SD.mkdir("/logs")) {
            Serial.println("[LogWriterTask] Successfully created /logs directory");
        } else {
            Serial.println("[LogWriterTask] FAILED to create /logs directory");
        }
    } else {
        Serial.println("Directory already exists");
    }
}

bool LogWriterTask::writeLogToFile(const String& logEntry) {
    if (!logFile || !logFile->isOpen()) {
        Serial.println("[LogWriterTask] Log file not available");
        return false;
    }
    
    Serial.print("[LogWriterTask] Writing to log file: ");
    Serial.println(logFile->getFilename());
    
    // Write the log entry (LogFile handles formatting and buffering)
    logFile->logLine(logEntry);
    
    Serial.println("[LogWriterTask] Log entry written to buffer");
    return true;
} 