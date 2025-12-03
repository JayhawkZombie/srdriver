#pragma once
#include <Arduino.h>
#include <functional>

class JsonChunkStreamer {
public:
    JsonChunkStreamer();
    void begin(const String& fullJson, const String& type = "FILE_LIST");
    void update(std::function<void(const String&)> sendChunk);
    bool isActive() const;
    void stop();
private:
    String _json;
    String _type;
    int _maxChunkSize;
    int _totalChunks;
    int _currentChunk;
    bool _active;
}; 