#include "LogFile.h"

LogFile::LogFile(const String& filename, FlushMode mode)
    : filename(filename)
    , bufferSize(512)
    , autoFlushThreshold(256)
    , flushMode(mode)
    , fileOpen(false)
    , inMultiLineEntry(false)
    , entryStartTime(0)
    , currentLevel(LogManager::INFO) {
}

LogFile::~LogFile() {
    if (fileOpen) {
        flush();
        close();
    }
}

bool LogFile::open() {
    if (fileOpen) return true;
    
    // Ensure directory exists
    String dir = filename.substring(0, filename.lastIndexOf('/'));
    if (dir.length() > 0 && !SD.exists(dir.c_str())) {
        SD.mkdir(dir.c_str());
    }
    
    file = SD.open(filename.c_str(), FILE_APPEND);
    fileOpen = file;
    
    if (fileOpen) {
        Serial.print("[LogFile] Opened log file: ");
        Serial.println(filename);
    } else {
        Serial.print("[LogFile] Failed to open log file: ");
        Serial.println(filename);
    }
    
    return fileOpen;
}

void LogFile::close() {
    if (fileOpen) {
        flush();
        file.close();
        fileOpen = false;
        Serial.print("[LogFile] Closed log file: ");
        Serial.println(filename);
    }
}

void LogFile::log(LogManager::LogLevel level, const String& message) {
    if (!fileOpen && !open()) return;
    
    // Format: [timestamp] LEVEL: message
    String formattedEntry = "[" + String(millis()) + "] " + levelToString(level) + ": " + message + "\n";
    
    if (flushMode == FlushMode::IMMEDIATE) {
        writeToFile(formattedEntry);
    } else {
        buffer += formattedEntry;
        checkAutoFlush();
    }
}

void LogFile::logLine(const String& line) {
    if (!fileOpen && !open()) return;
    
    String formattedLine = line + "\n";
    
    if (flushMode == FlushMode::IMMEDIATE) {
        writeToFile(formattedLine);
    } else {
        buffer += formattedLine;
        checkAutoFlush();
    }
}

void LogFile::beginEntry(LogManager::LogLevel level, const String& message) {
    if (inMultiLineEntry) {
        endEntry(); // Close previous entry if still open
    }
    
    currentLevel = level;
    entryStartTime = millis();
    inMultiLineEntry = true;
    
    // Write the first line
    log(level, message);
}

void LogFile::addLine(const String& line) {
    if (!inMultiLineEntry) {
        Serial.println("[LogFile] Warning: addLine() called without beginEntry()");
        return;
    }
    
    // Add indented continuation line
    String indentedLine = "    " + line;
    logLine(indentedLine);
}

void LogFile::endEntry() {
    if (!inMultiLineEntry) return;
    
    inMultiLineEntry = false;
    // Entry is already complete, no additional formatting needed
}

void LogFile::flush() {
    if (!fileOpen || buffer.length() == 0) return;
    
    writeToFile(buffer);
    buffer.clear();
}

void LogFile::setBufferSize(size_t size) {
    bufferSize = size;
}

void LogFile::setAutoFlushThreshold(size_t threshold) {
    autoFlushThreshold = threshold;
}

size_t LogFile::getFileSize() const {
    if (!fileOpen) return 0;
    return file.size();
}

void LogFile::writeToFile(const String& data) {
    if (!fileOpen) return;
    
    file.print(data);
    file.flush(); // Ensure data is written to SD card
}

String LogFile::levelToString(LogManager::LogLevel level) const {
    switch (level) {
        case LogManager::DEBUG: return "DEBUG";
        case LogManager::INFO:  return "INFO";
        case LogManager::WARN:  return "WARN";
        case LogManager::ERROR: return "ERROR";
        default:               return "UNKNOWN";
    }
}

void LogFile::checkAutoFlush() {
    if (flushMode == FlushMode::AUTO_FLUSH && buffer.length() >= autoFlushThreshold) {
        flush();
    }
} 