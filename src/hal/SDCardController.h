#pragma once
#include "PlatformConfig.h"
#if SUPPORTS_SD_CARD

#include <Arduino.h>
#include <functional>

// Opaque file handle declaration
struct SDCardFileHandle;

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
    // File operations with opaque handles
    virtual SDCardFileHandle* open(const char* path, const char* mode) = 0;
    virtual void close(SDCardFileHandle* handle) = 0;
    virtual size_t read(SDCardFileHandle* handle, uint8_t* buffer, size_t length) = 0;
    virtual size_t write(SDCardFileHandle* handle, const uint8_t* buffer, size_t length) = 0;
    virtual bool seek(SDCardFileHandle* handle, long position) = 0;
    virtual long position(SDCardFileHandle* handle) = 0;
    virtual long size(SDCardFileHandle* handle) = 0;
    virtual bool available(SDCardFileHandle* handle) = 0;
    // Convenience methods for whole file operations
    virtual bool writeFile(const char* path, const char* data) = 0;
    virtual String readFile(const char* path) = 0;
    virtual bool appendFile(const char* path, const char* data) = 0;
    // Directory operations
    virtual SDCardFileHandle* openDir(const char* path) = 0;
    virtual bool listDir(const char* path, std::function<void(const char*, bool, size_t)> callback) = 0;
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
    SDCardFileHandle* open(const char* path, const char* mode) override { return nullptr; }
    void close(SDCardFileHandle* handle) override { }
    size_t read(SDCardFileHandle* handle, uint8_t* buffer, size_t length) override { return 0; }
    size_t write(SDCardFileHandle* handle, const uint8_t* buffer, size_t length) override { return 0; }
    bool seek(SDCardFileHandle* handle, long position) override { return false; }
    long position(SDCardFileHandle* handle) override { return 0; }
    long size(SDCardFileHandle* handle) override { return 0; }
    bool available(SDCardFileHandle* handle) override { return false; }
    bool writeFile(const char* path, const char* data) override { return false; }
    String readFile(const char* path) override { return String(); }
    bool appendFile(const char* path, const char* data) override { return false; }
    SDCardFileHandle* openDir(const char* path) override { return nullptr; }
    bool listDir(const char* path, std::function<void(const char*, bool, size_t)> callback) override { return false; }
};

// Global SD card controller instance
extern SDCardController* g_sdCardController;

#endif // SUPPORTS_SD_CARD 