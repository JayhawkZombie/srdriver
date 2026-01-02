#pragma once

#ifdef PLATFORM_CROW_PANEL

#include <SD.h>
#include <SPI.h>
#include <functional>
#include "../SDCardController.h"

// SD Card pin definitions for CrowPanel
#define CROW_PANEL_SD_MOSI 11
#define CROW_PANEL_SD_MISO 13
#define CROW_PANEL_SD_CLK  12
#define CROW_PANEL_SD_CS   10

struct SDCardFileHandle {
    File file;
    bool isDirectory;
    SDCardFileHandle(File f, bool dir = false) : file(f), isDirectory(dir) {}
};

class CrowPanelSDCardController : public SDCardController {
private:
    bool m_available = false;
    bool m_spiInitialized = false;

    void initializeSPI()
    {
        if (m_spiInitialized) return;

        // Configure CS pin
        pinMode(CROW_PANEL_SD_CS, OUTPUT);
        digitalWrite(CROW_PANEL_SD_CS, HIGH); // CS high = inactive

        // Initialize SPI for SD card with custom pins
        SPI.begin(CROW_PANEL_SD_CLK, CROW_PANEL_SD_MISO, CROW_PANEL_SD_MOSI, CROW_PANEL_SD_CS);
        SPI.setFrequency(10000000); // 10MHz for SD card

        m_spiInitialized = true;
    }

public:
    bool begin(int pin) override
    {
        Serial.println("[CrowPanelSDCardController] Initializing SD card with custom SPI pins");

        // Initialize SPI with custom pins first
        initializeSPI();

        // Now initialize SD card (pin parameter is ignored since we use custom SPI)
        m_available = SD.begin(CROW_PANEL_SD_CS);

        if (m_available)
        {
            Serial.println("[CrowPanelSDCardController] SD card initialized successfully");
        }
        else
        {
            Serial.println("[CrowPanelSDCardController] SD card initialization failed");
        }

        return m_available;
    }

    bool isAvailable() const override { return m_available; }

    bool exists(const char *path) override
    {
        return m_available ? SD.exists(path) : false;
    }

    bool mkdir(const char *path) override
    {
        return m_available ? SD.mkdir(path) : false;
    }

    bool remove(const char *path) override
    {
        return m_available ? SD.remove(path) : false;
    }

    bool rename(const char *oldPath, const char *newPath) override
    {
        return m_available ? SD.rename(oldPath, newPath) : false;
    }

    SDCardFileHandle *open(const char *path, const char *mode) override
    {
        if (!m_available) return nullptr;
        File file = SD.open(path, mode);
        if (!file) return nullptr;
        return new SDCardFileHandle(file, false);
    }

    void close(SDCardFileHandle *handle) override
    {
        if (handle)
        {
            handle->file.close();
            delete handle;
        }
    }

    size_t read(SDCardFileHandle *handle, uint8_t *buffer, size_t length) override
    {
        if (!handle || handle->isDirectory) return 0;
        return handle->file.read(buffer, length);
    }

    size_t write(SDCardFileHandle *handle, const uint8_t *buffer, size_t length) override
    {
        if (!handle || handle->isDirectory) return 0;
        return handle->file.write(buffer, length);
    }

    bool seek(SDCardFileHandle *handle, long position) override
    {
        if (!handle || handle->isDirectory) return false;
        return handle->file.seek(position);
    }

    long position(SDCardFileHandle *handle) override
    {
        if (!handle || handle->isDirectory) return 0;
        return handle->file.position();
    }

    long size(SDCardFileHandle *handle) override
    {
        if (!handle || handle->isDirectory) return 0;
        return handle->file.size();
    }

    bool available(SDCardFileHandle *handle) override
    {
        if (!handle || handle->isDirectory) return false;
        return handle->file.available();
    }

    bool writeFile(const char *path, const char *data) override
    {
        if (!m_available) return false;
        File file = SD.open(path, FILE_WRITE);
        if (!file) return false;
        bool result = file.print(data);
        file.close();
        return result;
    }

    String readFile(const char *path) override
    {
        if (!m_available) return String();
        File file = SD.open(path, FILE_READ);
        if (!file) return String();
        String content = file.readString();
        file.close();
        return content;
    }

    bool appendFile(const char *path, const char *data) override
    {
        if (!m_available) return false;
        File file = SD.open(path, FILE_APPEND);
        if (!file) return false;
        bool result = file.print(data);
        file.close();
        return result;
    }

    SDCardFileHandle *openDir(const char *path) override
    {
        if (!m_available) return nullptr;
        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) return nullptr;
        return new SDCardFileHandle(dir, true);
    }

    bool listDir(const char *path, std::function<void(const char *, bool, size_t)> callback) override
    {
        if (!m_available) return false;
        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) return false;
        File file = dir.openNextFile();
        while (file)
        {
            if (callback) callback(file.name(), file.isDirectory(), file.size());
            file = dir.openNextFile();
        }
        dir.close();
        return true;
    }
};

#endif // PLATFORM_CROW_PANEL
