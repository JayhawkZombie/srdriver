#include "FileParser.h"

FileParser::FileParser(const String& filename, Mode mode)
    : filename(filename)
    , mode(mode) {
    
    // Ensure directory exists for write/append modes
    if (mode != Mode::READ) {
        String dir = filename.substring(0, filename.lastIndexOf('/'));
        if (dir.length() > 0 && !SD.exists(dir.c_str())) {
            SD.mkdir(dir.c_str());
        }
    }
    
    // Open file in appropriate mode
    if (mode == Mode::READ) {
        file = SD.open(filename.c_str(), FILE_READ);
    } else if (mode == Mode::WRITE) {
        file = SD.open(filename.c_str(), FILE_WRITE);
    } else { // APPEND
        file = SD.open(filename.c_str(), FILE_APPEND);
    }
    
    if (file) {
        Serial.print("[FileParser] Opened ");
        Serial.print(filename);
        Serial.print(" in ");
        Serial.println(mode == Mode::READ ? "READ" : mode == Mode::WRITE ? "WRITE" : "APPEND");
    } else {
        Serial.print("[FileParser] Failed to open ");
        Serial.println(filename);
    }
}

FileParser::~FileParser() {
    if (file) {
        close();
    }
}

void FileParser::close() {
    if (file) {
        flush();
        file.close();
        Serial.print("[FileParser] Closed ");
        Serial.println(filename);
    }
}

// Input operators (>>) - for reading
FileParser& FileParser::operator>>(int& value) {
    if (mode != Mode::READ || !file) return *this;
    
    String token = getNextToken();
    if (token.length() > 0) {
        value = token.toInt();
    }
    return *this;
}

FileParser& FileParser::operator>>(float& value) {
    if (mode != Mode::READ || !file) return *this;
    
    String token = getNextToken();
    if (token.length() > 0) {
        value = token.toFloat();
    }
    return *this;
}

FileParser& FileParser::operator>>(double& value) {
    if (mode != Mode::READ || !file) return *this;
    
    String token = getNextToken();
    if (token.length() > 0) {
        value = token.toFloat(); // Arduino doesn't have double, use float
    }
    return *this;
}

FileParser& FileParser::operator>>(String& value) {
    if (mode != Mode::READ || !file) return *this;
    
    value = getNextToken();
    return *this;
}

FileParser& FileParser::operator>>(char& value) {
    if (mode != Mode::READ || !file) return *this;
    
    if (file.available()) {
        value = file.read();
    }
    return *this;
}

// Output operators (<<) - for writing
FileParser& FileParser::operator<<(int value) {
    if (mode == Mode::READ || !file) return *this;
    
    file.print(value);
    return *this;
}

FileParser& FileParser::operator<<(float value) {
    if (mode == Mode::READ || !file) return *this;
    
    file.print(value);
    return *this;
}

FileParser& FileParser::operator<<(double value) {
    if (mode == Mode::READ || !file) return *this;
    
    file.print(value);
    return *this;
}

FileParser& FileParser::operator<<(const String& value) {
    if (mode == Mode::READ || !file) return *this;
    
    file.print(value);
    return *this;
}

FileParser& FileParser::operator<<(const char* value) {
    if (mode == Mode::READ || !file) return *this;
    
    file.print(value);
    return *this;
}

FileParser& FileParser::operator<<(char value) {
    if (mode == Mode::READ || !file) return *this;
    
    file.print(value);
    return *this;
}

// Helper methods
String FileParser::readLine() {
    if (mode != Mode::READ || !file) return "";
    
    return file.readStringUntil('\n');
}

void FileParser::writeLine(const String& line) {
    if (mode == Mode::READ || !file) return;
    
    file.println(line);
}

void FileParser::flush() {
    if (file) {
        file.flush();
    }
}

// Private helper methods
String FileParser::getNextToken() {
    if (mode != Mode::READ || !file) return "";
    
    skipWhitespace();
    
    String token = "";
    while (file.available()) {
        char c = file.peek();
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            break;
        }
        token += (char)file.read();
    }
    
    return token;
}

void FileParser::skipWhitespace() {
    if (mode != Mode::READ || !file) return;
    
    while (file.available()) {
        char c = file.peek();
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            break;
        }
        file.read(); // Skip this character
    }
} 