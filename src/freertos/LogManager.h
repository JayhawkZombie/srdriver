#pragma once

#include <vector>
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
#if SUPPORTS_SD_CARD
        // Archive existing log file if it exists
        archiveCurrentLog();
#endif
        _initialized = true;
    }
    
    /**
     * Archive the current log file with timestamp
     */
    void archiveCurrentLog() {
        debugComponent("LogManager", "Archiving current log file");
#if SUPPORTS_SD_CARD
        extern SDCardController* g_sdCardController;
        if (!g_sdCardController) return;
        
        // Check if current log file exists
        if (g_sdCardController->exists("/logs/srdriver.log")) {
            // Create archives directory if it doesn't exist
            if (!g_sdCardController->exists("/logs/archives")) {
                g_sdCardController->mkdir("/logs/archives");
            }
            
            // Generate readable timestamp for archive filename
            unsigned long uptime = millis();
            unsigned long seconds = uptime /1000;
            unsigned long minutes = seconds / 60;
            unsigned long hours = minutes / 60;
            unsigned long days = hours /24;
            String timestamp = String(days) + "d" + String(hours % 24) + "h" + String(minutes % 60) + "m";

            String archiveName = "/logs/srdriver_old.log";
            // Remove the old "old" log file, if it exists
            if (g_sdCardController->exists(archiveName.c_str())) {
                debugComponent("LogManager", "Removing old log file");
                g_sdCardController->remove(archiveName.c_str());
            }
            
            // move current log to archive
            if (g_sdCardController->rename("/logs/srdriver.log", archiveName.c_str())) {
                debugComponentPrintf("LogManager", "Archived log file: %s", archiveName.c_str());

                // Create empty new log file
                g_sdCardController->remove("/logs/srdriver.log");
                g_sdCardController->writeFile("/logs/srdriver.log", "");
            } else {
                errorComponent("LogManager", "Failed to archive log file");


                // Then we'll just delete the old one and make a new one
                if (!g_sdCardController->remove("/logs/srdriver.log")) {
                    errorComponent("LogManager", "Failed to delete old log file, not sure what to do here lol");
                }
                if (!g_sdCardController->writeFile("/logs/srdriver.log", "")) {
                    errorComponent("LogManager", "Failed to create new log file, not sure what to do here lol wtf");
                }

            }
        }
#endif
    }
    
    /**
     * Manually trigger log rotation (useful for testing or maintenance)
     */
    void rotateLogs() {
#if SUPPORTS_SD_CARD
        if (_initialized) {
            archiveCurrentLog();
        }
#endif
    }
    
    /**
     * Clean up old log archives (keep only the most recent ones)
     * @param keepCount Number of most recent archives to keep
     */
    void cleanupOldArchives(int keepCount = 5) {
#if SUPPORTS_SD_CARD
        extern SDCardController* g_sdCardController;
        if (!g_sdCardController) return;
        
        // This is a simple implementation - in a real system you might want
        // to list files, sort by modification time, and delete the oldest
        debugComponentPrintf("LogManager", "Cleanup: Keeping %d most recent log archives", keepCount);
        // TODO: Implement file listing and cleanup logic
#endif
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
    
    /**
     * NEW: Component-aware logging methods
     */
    void debugComponent(const char* component, const char* message) {
        log(LogMessage::debug(component, message));
    }
    
    void infoComponent(const char* component, const char* message) {
        log(LogMessage::info(component, message));
    }
    
    void warnComponent(const char* component, const char* message) {
        log(LogMessage::warn(component, message));
    }
    
    void errorComponent(const char* component, const char* message) {
        log(LogMessage::error(component, message));
    }
    
    void debugComponent(const char* component, const String& message) {
        log(LogMessage::debug(component, message));
    }
    void debugComponentPrintf(const char *component, const char *format, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(LogMessage::debug(component, buffer));
    }
    
    void infoComponent(const char* component, const String& message) {
        log(LogMessage::info(component, message));
    }

    void infoComponentPrintf(const char *component, const char *format, ...)
    {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(LogMessage::info(component, buffer));
    }
    
    void warnComponent(const char* component, const String& message) {
        log(LogMessage::warn(component, message));
    }

    void warnComponentPrintf(const char *component, const char *format, ...)
    {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(LogMessage::warn(component, buffer));
    }
    
    void errorComponent(const char* component, const String& message) {
        log(LogMessage::error(component, message));
    }

    void errorComponentPrintf(const char *component, const char *format, ...)
    {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(LogMessage::error(component, buffer));
    }
    
    /**
     * NEW: Component filtering methods
     */
    void setComponentFilter(const std::vector<String>& components) {
        _allowedComponents = components;
        _componentFilteringEnabled = !components.empty();
    }
    
    void enableAllComponents() {
        _componentFilteringEnabled = false;
        _allowedComponents.clear();
    }
    
    void addComponent(const String& component) {
        _allowedComponents.push_back(component);
        _componentFilteringEnabled = true;
    }
    
    void removeComponent(const String& component) {
        for (auto it = _allowedComponents.begin(); it != _allowedComponents.end(); ++it) {
            if (*it == component) {
                _allowedComponents.erase(it);
                break;
            }
        }
        if (_allowedComponents.empty()) {
            _componentFilteringEnabled = false;
        }
    }
    
    /**
     * NEW: Timestamp filtering methods (filter out old logs)
     */
    void setTimestampFilter(uint32_t minTimestamp) {
        _minTimestamp = minTimestamp;
        _timestampFilteringEnabled = true;
    }
    
    void disableTimestampFilter() {
        _timestampFilteringEnabled = false;
        _minTimestamp = 0;
    }
    
    void setNewLogsOnly() {
        _minTimestamp = millis();
        _timestampFilteringEnabled = true;
    }
    
    /**
     * NEW: Get current filtering status
     */
    bool isComponentFilteringEnabled() const {
        return _componentFilteringEnabled;
    }
    
    bool isTimestampFilteringEnabled() const {
        return _timestampFilteringEnabled;
    }
    
    std::vector<String> getAllowedComponents() const {
        return _allowedComponents;
    }
    
    uint32_t getMinTimestamp() const {
        return _minTimestamp;
    }

    bool isLevelFilteringEnabled() const {
        return _levelFilteringEnabled;
    }

    void setLevelFilter(const std::vector<String>& levels) {
        _allowedLevels = levels;
        _levelFilteringEnabled = !levels.empty();
    }

    void enableAllLevels() {
        _levelFilteringEnabled = false;
        _allowedLevels.clear();
    }
    
    void addLevel(const String& level) {
        _allowedLevels.push_back(level);
        _levelFilteringEnabled = true;
    }

    void removeLevel(const String& level) {
        for (auto it = _allowedLevels.begin(); it != _allowedLevels.end(); ++it) {
            if (*it == level) {
                _allowedLevels.erase(it);
                break;
            }
        }
        if (_allowedLevels.empty()) {
            _levelFilteringEnabled = false;
        }
    }
    
    std::vector<String> getAllowedLevels() const {
        return _allowedLevels;
    }
    
private:
    LogManager() : _initialized(false), 
                   _componentFilteringEnabled(false),
                   _timestampFilteringEnabled(false),
                   _minTimestamp(0) {}
    
    /**
     * NEW: Check if message should be logged based on filters
     */
    bool shouldLog(const LogMessage& msg) {
        // Check component filter
        if (_componentFilteringEnabled) {
            bool componentAllowed = false;
            for (const String& allowed : _allowedComponents) {
                if (strcmp(msg.component, allowed.c_str()) == 0) {
                    componentAllowed = true;
                    break;
                }
            }
            if (!componentAllowed) {
                return false;
            }
        }

        if (_levelFilteringEnabled) {
            bool levelAllowed = false;
            for (const String& allowed : _allowedLevels) {
                if (strcmp(msg.getLevelString(), allowed.c_str()) == 0) {
                    levelAllowed = true;
                    break;
                }
            }
            if (!levelAllowed) {
                return false;
            }
        }
        
        // Check timestamp filter
        if (_timestampFilteringEnabled && msg.timestamp < _minTimestamp) {
            return false;
        }
        
        return true;
    }
    
    void log(const LogMessage& msg) {
        // NEW: Check if this message should be logged
        if (!shouldLog(msg)) {
            return; // Skip logging if filtered out
        }
        
        // Always output to Serial for immediate debugging
        if (strlen(msg.component) > 0) {
            // Component-aware logging
            Serial.printf("[%s] [%-5s]: {%s} %s\n", 
                String(msg.timestamp).c_str(),
                        msg.getLevelString(), 
                         msg.component,
                         msg.message);
        } else {
            // Legacy logging (no component)
                Serial.printf("[%s] [%-5s]: %s\n", 
                         String(msg.timestamp).c_str(),
                         msg.getLevelString(), 
                         msg.message);
        }
        
        // Write to SD card if available
#if SUPPORTS_SD_CARD
        if (_initialized) {
            extern SDCardController* g_sdCardController;
            if (g_sdCardController) {
                // Format log entry
                String logEntry;
                if (strlen(msg.component) > 0) {
                    // Component-aware log entry
                    logEntry = String("[") + String(msg.timestamp) + "] [" + 
                              String(msg.component) + "] " + 
                              msg.getLevelString() + ": " + msg.message + "\n";
                } else {
                    // Legacy log entry
                    logEntry = String("[") + String(msg.timestamp) + "] " + 
                              msg.getLevelString() + ": " + msg.message + "\n";
                }
                
                // Write to log file
                g_sdCardController->appendFile("/logs/srdriver.log", logEntry.c_str());
            }
        }
#endif
    }
    
    bool _initialized;
    
    // NEW: Filtering state variables
    bool _componentFilteringEnabled;
    bool _timestampFilteringEnabled;
    bool _levelFilteringEnabled;
    std::vector<String> _allowedComponents;
    std::vector<String> _allowedLevels;
    uint32_t _minTimestamp;
};

// Convenience macros for easy logging (legacy - backward compatible)
#define LOG_DEBUG(msg) LogManager::getInstance().debug(msg)
#define LOG_INFO(msg)  LogManager::getInstance().info(msg)
#define LOG_WARN(msg)  LogManager::getInstance().warn(msg)
#define LOG_ERROR(msg) LogManager::getInstance().error(msg)
#define LOG_PRINTF(fmt, ...) LogManager::getInstance().printf(fmt, ##__VA_ARGS__)
#define LOG_DEBUGF(fmt, ...) LogManager::getInstance().debugf(fmt, ##__VA_ARGS__)
#define LOG_INFOF(fmt, ...) LogManager::getInstance().printf(fmt, ##__VA_ARGS__)
#define LOG_WARNF(fmt, ...) LogManager::getInstance().warnf(fmt, ##__VA_ARGS__)
#define LOG_ERRORF(fmt, ...) LogManager::getInstance().errorf(fmt, ##__VA_ARGS__)

// NEW: Component-aware logging macros
#define LOG_DEBUG_COMPONENT(comp, msg) LogManager::getInstance().debugComponent(comp, msg)
#define LOG_DEBUGF_COMPONENT(comp, fmt, ...) LogManager::getInstance().debugComponentPrintf(comp, fmt, ##__VA_ARGS__)
#define LOG_INFO_COMPONENT(comp, msg)  LogManager::getInstance().infoComponent(comp, msg)
#define LOG_INFOF_COMPONENT(comp, fmt, ...) LogManager::getInstance().infoComponentPrintf(comp, fmt, ##__VA_ARGS__)
#define LOG_WARN_COMPONENT(comp, msg)  LogManager::getInstance().warnComponent(comp, msg)
#define LOG_WARNF_COMPONENT(comp, fmt, ...) LogManager::getInstance().warnComponentPrintf(comp, fmt, ##__VA_ARGS__)
#define LOG_ERROR_COMPONENT(comp, msg) LogManager::getInstance().errorComponent(comp, msg)
#define LOG_ERRORF_COMPONENT(comp, fmt, ...) LogManager::getInstance().errorComponentPrintf(comp, fmt, ##__VA_ARGS__)

// NEW: Filtering control macros
#define LOG_SET_COMPONENT_FILTER(components) LogManager::getInstance().setComponentFilter(components)
#define LOG_ENABLE_ALL_COMPONENTS() LogManager::getInstance().enableAllComponents()
#define LOG_ADD_COMPONENT(comp) LogManager::getInstance().addComponent(comp)
#define LOG_REMOVE_COMPONENT(comp) LogManager::getInstance().removeComponent(comp)
#define LOG_SET_LEVEL_FILTER(levels) LogManager::getInstance().setLevelFilter(levels)
#define LOG_ENABLE_ALL_LEVELS() LogManager::getInstance().enableAllLevels()
#define LOG_ADD_LEVEL(level) LogManager::getInstance().addLevel(level)
#define LOG_REMOVE_LEVEL(level) LogManager::getInstance().removeLevel(level)
#define LOG_SET_NEW_LOGS_ONLY() LogManager::getInstance().setNewLogsOnly()
#define LOG_DISABLE_TIMESTAMP_FILTER() LogManager::getInstance().disableTimestampFilter() 