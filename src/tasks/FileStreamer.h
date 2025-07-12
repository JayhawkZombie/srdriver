#pragma once
#include <SD.h>

class FileStreamer {
public:
    FileStreamer();
    
    bool begin(const char* filename);
    void update();
    
    bool isActive() const;
    const uint8_t* getBuffer() const;
    size_t getBufferSize() const;
    
private:
    static const size_t BUFFER_SIZE = 512;
    bool active;
    File file;
    uint8_t buffer[BUFFER_SIZE];
    size_t bufferSize;
}; 