#pragma once

#include "SRQueue.h"
#include "LogMessage.h"
#include "PlatformConfig.h"
#if SUPPORTS_SD_CARD
#include "hal/SDCardController.h"
#endif

/**
 * LogManager - Global logging interface
 * 
 * Provides a singleton interface for logging throughout the application.
 * Writes directly to SD card using platform abstraction.
 */
class LogManager {
public:
    /**
     * Get the global LogManager instance
     */
    static LogManager& getInstance() {
        static LogManager instance;
        return instance;
    }
    
    /**
     * Initialize the logging system
     */
    void initialize() {
        _initialized = true;
    }
    
    /**
     * Log a message at DEBUG level
     */
    void debug(const char* message) {
        log(LogMessage::debug(message));
    }
    
    /**
     * Log a message at INFO level
     */
    void info(const char* message) {
        log(LogMessage::info(message));
    }
    
    /**
     * Log a message at WARN level
     */
    void warn(const char* message) {
        log(LogMessage::warn(message));
    }
    
    /**
     * Log a message at ERROR level
     */
    void error(const char* message) {
        log(LogMessage::error(message));
    }
    
    /**
     * Log a message at DEBUG level (String version)
     */
    void debug(const String& message) {
        log(LogMessage::debug(message));
    }
    
    /**
     * Log a message at INFO level (String version)
     */
    void info(const String& message) {
        log(LogMessage::info(message));
    }
    
    /**
     * Log a message at WARN level (String version)
     */
    void warn(const String& message) {
        log(LogMessage::warn(message));
    }
    
    /**
     * Log a message at ERROR level (String version)
     */
    void error(const String& message) {
        log(LogMessage::error(message));
    }
    
    /**
     * Log a formatted message at INFO level
     */
    void printf(const char* format, ...) {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(LogMessage::info(buffer));
    }
    
    /**
     * Log a formatted message at DEBUG level
     */
    void debugf(const char* format, ...) {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(LogMessage::debug(buffer));
    }
    
    /**
     * Log a formatted message at WARN level
     */
    void warnf(const char* format, ...) {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(LogMessage::warn(buffer));
    }
    
    /**
     * Log a formatted message at ERROR level
     */
    void errorf(const char* format, ...) {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(LogMessage::error(buffer));
    }
    
    /**
     * Check if logging is available
     */
    bool isAvailable() const {
        return _initialized;
    }

private:
    LogManager() : _initialized(false) {}
    
    void log(const LogMessage& msg) {
        // Always output to Serial for immediate debugging
        Serial.printf("[%s] %s: %s\n", 
                     msg.getLevelString(), 
                     String(msg.timestamp).c_str(),
                     msg.message);
        
        // Write to SD card if available
#if SUPPORTS_SD_CARD
        if (_initialized) {
            extern SDCardController* g_sdCardController;
            if (g_sdCardController) {
                // Format log entry
                String logEntry = String("[") + String(msg.timestamp) + "] " + 
                                msg.getLevelString() + ": " + msg.message + "\n";
                
                // Write to log file
                g_sdCardController->appendFile("/logs/srdriver.log", logEntry.c_str());
            }
        }
#endif
    }
    
    bool _initialized;
};

// Convenience macros for easy logging
#define LOG_DEBUG(msg) LogManager::getInstance().debug(msg)
#define LOG_INFO(msg)  LogManager::getInstance().info(msg)
#define LOG_WARN(msg)  LogManager::getInstance().warn(msg)
#define LOG_ERROR(msg) LogManager::getInstance().error(msg)
#define LOG_PRINTF(fmt, ...) LogManager::getInstance().printf(fmt, ##__VA_ARGS__)
#define LOG_DEBUGF(fmt, ...) LogManager::getInstance().debugf(fmt, ##__VA_ARGS__)
#define LOG_INFOF(fmt, ...) LogManager::getInstance().printf(fmt, ##__VA_ARGS__)
#define LOG_WARNF(fmt, ...) LogManager::getInstance().warnf(fmt, ##__VA_ARGS__)
#define LOG_ERRORF(fmt, ...) LogManager::getInstance().errorf(fmt, ##__VA_ARGS__) 