#include "FileStreamer.h"

FileStreamer::FileStreamer() : active(false), file(), bufferSize(0) {
}

bool FileStreamer::begin(const char* filename) {
    if (active) return false;
    file = SD.open(filename);
    if (!file) return false;
    active = true;
    bufferSize = 0;
    return true;
}

void FileStreamer::update() {
    if (!active || !file) return;
    
    if (file.available()) {
        // Read a chunk of data
        int bytesRead = file.read(buffer, BUFFER_SIZE);
        if (bytesRead > 0) {
            bufferSize = bytesRead;
        }
    } else {
        // File is done
        file.close();
        active = false;
    }
}

bool FileStreamer::isActive() const { 
    return active; 
}

const uint8_t* FileStreamer::getBuffer() const { 
    return buffer; 
}

size_t FileStreamer::getBufferSize() const { 
    return bufferSize; 
} 