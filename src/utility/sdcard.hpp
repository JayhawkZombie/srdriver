#pragma once

// #include <SD.h>
// #include <SPI.h>
#include "SdFat.h"
#include "FileReader.hpp"

class SDCard
{

    SdFat32 sd;
    FatFile root;
    FileReader fileReader;

public:
    SDCard() = default;
    ~SDCard()
    {
        if (fileReader.isOpen())
        {
            fileReader.close();
        }
        if (root.isOpen())
        {
            root.close();
        }
    }

    bool init(uint8_t chipSelect)
    {
        if (!sd.begin(chipSelect, SD_SCK_MHZ(50)))
        {
            Serial.println("Failed to initialize SD card");
            return false;
        }

        if (!root.open("/"))
        {
            Serial.println("Failed to open root directory");
            return false;
        }

        root.ls();
        root.close();

        return true;
    }

    bool openFile(const char *filename) {
        return fileReader.open(filename, sd);
    }

    int readNextBytes(char *buffer, size_t size) {
        return fileReader.readBytes(buffer, size);
    }

    void closeFile() {
        fileReader.close();
    }

};
