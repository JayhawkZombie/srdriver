#pragma once
#include "PlatformConfig.h"
#if SUPPORTS_SD_CARD
#include <SD.h>
#include <WString.h>

class FileParser {
public:
    enum class Mode {
        READ,
        WRITE,
        APPEND
    };

    FileParser(const String& filename, Mode mode = Mode::READ);
    ~FileParser();
    
    // Check if file is open and ready
    bool isOpen() { return file && file.available(); }
    bool good() const { return file; }
    
    // Close the file
    void close();
    
    // Input operators (>>) - for reading
    FileParser& operator>>(int& value);
    FileParser& operator>>(float& value);
    FileParser& operator>>(double& value);
    FileParser& operator>>(String& value);
    FileParser& operator>>(char& value);
    
    // Output operators (<<) - for writing
    FileParser& operator<<(int value);
    FileParser& operator<<(float value);
    FileParser& operator<<(double value);
    FileParser& operator<<(const String& value);
    FileParser& operator<<(const char* value);
    FileParser& operator<<(char value);
    
    // Helper methods
    String readLine();
    void writeLine(const String& line);
    void flush();

private:
    File file;
    String filename;
    Mode mode;
    
    // Helper to get next token (space or newline separated)
    String getNextToken();
    void skipWhitespace();
}; 
#endif