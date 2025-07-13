#include "JsonChunkStreamer.h"
#include <ArduinoJson.h>

JsonChunkStreamer::JsonChunkStreamer()
    : _maxChunkSize(400), _active(false) {}

void JsonChunkStreamer::begin(const String& fullJson, const String& type) {
    _json = fullJson;
    _type = type;
    _currentChunk = 0;
    _totalChunks = (_json.length() + _maxChunkSize - 1) / _maxChunkSize;
    _active = true;
}

void JsonChunkStreamer::update(std::function<void(const String&)> sendChunk) {
    if (!_active) return;
    if (_currentChunk >= _totalChunks) {
        _active = false;
        return;
    }
    int start = _currentChunk * _maxChunkSize;
    int end = min(start + _maxChunkSize, (int)_json.length());
    String chunk = _json.substring(start, end);

    DynamicJsonDocument doc(600);
    doc["type"] = _type;
    doc["seq"] = _currentChunk + 1;
    doc["total"] = _totalChunks;
    doc["payload"] = chunk;
    doc["end"] = (_currentChunk + 1 == _totalChunks);

    String envelope;
    serializeJson(doc, envelope);
    sendChunk(envelope);

    _currentChunk++;
    if (_currentChunk >= _totalChunks) {
        _active = false;
    }
}

bool JsonChunkStreamer::isActive() const { return _active; }
void JsonChunkStreamer::stop() { _active = false; } 