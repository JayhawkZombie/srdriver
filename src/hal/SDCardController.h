#pragma once

#include <Arduino.h>
#include "PlatformConfig.h"

class SDCardController {
public:
    virtual ~SDCardController() = default;
    
    // Core SD operations
    virtual bool begin(int pin) = 0;
    virtual bool isAvailable() const = 0;
    virtual bool exists(const char* path) = 0;
    virtual bool mkdir(const char* path) = 0;
    virtual bool remove(const char* path) = 0;
    virtual bool rename(const char* oldPath, const char* newPath) = 0;
    
    // File operations
    virtual File open(const char* path, const char* mode) = 0;
    virtual bool writeFile(const char* path, const char* data) = 0;
    virtual String readFile(const char* path) = 0;
    virtual bool appendFile(const char* path, const char* data) = 0;
    
    // Directory operations
    virtual File openDir(const char* path) = 0;
    virtual bool listDir(const char* path, void (*callback)(const char*)) = 0;
};

// Null implementation for unsupported platforms
class NullSDCardController : public SDCardController {
public:
    bool begin(int pin) override { return false; }
    bool isAvailable() const override { return false; }
    bool exists(const char* path) override { return false; }
    bool mkdir(const char* path) override { return false; }
    bool remove(const char* path) override { return false; }
    bool rename(const char* oldPath, const char* newPath) override { return false; }
    
    File open(const char* path, const char* mode) override { return File(); }
    bool writeFile(const char* path, const char* data) override { return false; }
    String readFile(const char* path) override { return String(); }
    bool appendFile(const char* path, const char* data) override { return false; }
    
    File openDir(const char* path) override { return File(); }
    bool listDir(const char* path, void (*callback)(const char*)) override { return false; }
};

// ESP32 implementation
#if SUPPORTS_SD_CARD
#include <SD.h>

class ESP32SDCardController : public SDCardController {
private:
    bool m_available = false;
    
public:
    bool begin(int pin) override {
        m_available = SD.begin(pin);
        return m_available;
    }
    
    bool isAvailable() const override { return m_available; }
    
    bool exists(const char* path) override {
        return m_available ? SD.exists(path) : false;
    }
    
    bool mkdir(const char* path) override {
        return m_available ? SD.mkdir(path) : false;
    }
    
    bool remove(const char* path) override {
        return m_available ? SD.remove(path) : false;
    }
    
    bool rename(const char* oldPath, const char* newPath) override {
        return m_available ? SD.rename(oldPath, newPath) : false;
    }
    
    File open(const char* path, const char* mode) override {
        return m_available ? SD.open(path, mode) : File();
    }
    
    bool writeFile(const char* path, const char* data) override {
        if (!m_available) return false;
        
        File file = SD.open(path, FILE_WRITE);
        if (!file) return false;
        
        bool result = file.print(data);
        file.close();
        return result;
    }
    
    String readFile(const char* path) override {
        if (!m_available) return String();
        
        File file = SD.open(path, FILE_READ);
        if (!file) return String();
        
        String content = file.readString();
        file.close();
        return content;
    }
    
    bool appendFile(const char* path, const char* data) override {
        if (!m_available) return false;
        
        File file = SD.open(path, FILE_APPEND);
        if (!file) return false;
        
        bool result = file.print(data);
        file.close();
        return result;
    }
    
    File openDir(const char* path) override {
        return m_available ? SD.open(path) : File();
    }
    
    bool listDir(const char* path, void (*callback)(const char*)) override {
        if (!m_available) return false;
        
        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) return false;
        
        File file = dir.openNextFile();
        while (file) {
            if (callback) callback(file.name());
            file = dir.openNextFile();
        }
        dir.close();
        return true;
    }
};
#endif

// Global SD card controller instance
extern SDCardController* g_sdCardController; 