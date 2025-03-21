#pragma once
#include "SdFat.h"

class FileReader {
private:
    FatFile file;
    char buffer[10]; // Buffer to hold the bytes read
    size_t bufferIndex = 0; // Current index in the buffer

public:
    FileReader() = default;

    bool open(const char *filename, SdFat32 &sd) {
        file = sd.open(filename, O_READ);
        return file ? true : false;
    }

    int readBytes(char *buffer, size_t size) {
        if (!file) {
            return -1; // File not open
        }
        return file.read(buffer, size);
    }

    void close() {
        if (file.isOpen()) {
            file.close();
        }
    }

    bool isOpen() {
        return file.isOpen();
    }

    // Template function to read data into a variable
    template <typename T>
    bool read(T &data) {
        static_assert(sizeof(T) <= sizeof(buffer), "Type size exceeds buffer size");
        
        if (bufferIndex + sizeof(T) > sizeof(buffer)) {
            // Not enough data in the buffer
            return false;
        }

        memcpy(&data, &buffer[bufferIndex], sizeof(T));
        bufferIndex += sizeof(T);
        return true;
    }

    // Overload the >> operator
    template <typename T>
    FileReader& operator>>(T &data) {
        if (bufferIndex == 0) {
            // Read new data into the buffer
            int bytesRead = readBytes(buffer, sizeof(buffer));
            if (bytesRead <= 0) {
                // Handle end of file or error
                return *this;
            }
            bufferIndex = 0; // Reset buffer index after reading
        }

        if (!read(data)) {
            // Handle read failure
            return *this;
        }

        return *this;
    }
};
