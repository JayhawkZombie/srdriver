#pragma once

#include <Arduino.h>

/**
 * Log levels
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

/**
 * LogMessage - Structure for passing log messages through queues
 * 
 * Supports different log levels and flexible message content.
 * Designed to be efficient for queue transmission.
 */
struct LogMessage {
    LogLevel level;
    uint32_t timestamp;
    char message[128];  // Fixed size for queue efficiency
    
    LogMessage() : level(LogLevel::INFO), timestamp(0) {
        message[0] = '\0';
    }
    
    LogMessage(LogLevel lvl, const char* msg) : level(lvl), timestamp(millis()) {
        strncpy(message, msg, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
    }
    
    LogMessage(LogLevel lvl, const String& msg) : level(lvl), timestamp(millis()) {
        strncpy(message, msg.c_str(), sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
    }
    
    // Helper constructors for different log levels
    static LogMessage debug(const char* msg) { return LogMessage(LogLevel::DEBUG, msg); }
    static LogMessage info(const char* msg) { return LogMessage(LogLevel::INFO, msg); }
    static LogMessage warn(const char* msg) { return LogMessage(LogLevel::WARN, msg); }
    static LogMessage error(const char* msg) { return LogMessage(LogLevel::ERROR, msg); }
    
    static LogMessage debug(const String& msg) { return LogMessage(LogLevel::DEBUG, msg); }
    static LogMessage info(const String& msg) { return LogMessage(LogLevel::INFO, msg); }
    static LogMessage warn(const String& msg) { return LogMessage(LogLevel::WARN, msg); }
    static LogMessage error(const String& msg) { return LogMessage(LogLevel::ERROR, msg); }
    
    // Get level as string
    const char* getLevelString() const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
}; 