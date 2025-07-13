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
    
    // Check if SD card is available before trying to create directories
    if (SD.exists("/")) {
        ensureLogsDirectory();
        Serial.println("[LogWriterTask] Logs directory ensured");
        
        // Get the log file path from LogManager
        LogManager& logManager = LogManager::getInstance();
        String logFilePath = logManager.getLogFile();
        
        // Create log file with auto-flush mode
        logFile = new LogFile(logFilePath, LogFile::FlushMode::AUTO_FLUSH);
        if (logFile->open()) {
            Serial.print("[LogWriterTask] Log file opened successfully: ");
            Serial.println(logFilePath);
        } else {
            Serial.print("[LogWriterTask] Failed to open log file: ");
            Serial.println(logFilePath);
        }
    } else {
        Serial.println("[LogWriterTask] No SD card available - logging to serial only");
        logFile = nullptr;
    }
    
    initialized = true;
    Serial.println("[LogWriterTask] Initialization complete");
}

void LogWriterTask::setLogFile(const String& filename) {
    if (logFile) {
        logFile->close();
        delete logFile;
    }
    
    // Check if SD card is available
    if (SD.exists("/")) {
        logFile = new LogFile(filename, LogFile::FlushMode::AUTO_FLUSH);
        if (logFile->open()) {
            Serial.print("[LogWriterTask] Switched to log file: ");
            Serial.println(filename);
        } else {
            Serial.print("[LogWriterTask] Failed to open log file: ");
            Serial.println(filename);
        }
    } else {
        Serial.println("[LogWriterTask] No SD card available - cannot switch log file");
        logFile = nullptr;
    }
}

// Add a method to refresh the log file (useful after rotation)
void LogWriterTask::refreshLogFile() {
    if (logFile) {
        logFile->close();
        delete logFile;
    }
    
    // Check if SD card is available
    if (SD.exists("/")) {
        LogManager& logManager = LogManager::getInstance();
        String logFilePath = logManager.getLogFile();
        
        logFile = new LogFile(logFilePath, LogFile::FlushMode::AUTO_FLUSH);
        if (logFile->open()) {
            Serial.print("[LogWriterTask] Refreshed log file: ");
            Serial.println(logFilePath);
        } else {
            Serial.print("[LogWriterTask] Failed to refresh log file: ");
            Serial.println(logFilePath);
        }
    } else {
        Serial.println("[LogWriterTask] No SD card available - cannot refresh log file");
        logFile = nullptr;
    }
}

void LogWriterTask::update() {
    if (!initialized) {
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
            
            bool writeSuccess = false;
            if (logFile && logFile->isOpen()) {
                writeSuccess = writeLogToFile(logEntry);
            } else {
                // SD card not available - just print to serial and mark as processed
                Serial.print("[LogWriterTask] SD card not available, printing to serial: ");
                Serial.println(logEntry);
                writeSuccess = true; // Consider it "successful" since we handled it
            }
            
            if (writeSuccess) {
                logManager.markLogProcessed();
                Serial.println("[LogWriterTask] Log entry processed successfully");
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