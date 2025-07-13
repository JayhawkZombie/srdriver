#pragma once
#include <Arduino.h>
#include <vector>
#include <utility>

// Split a string into tokens by delimiter (default: space)
std::vector<String> splitString(const String& input, char delimiter = ' ', bool trimTokens = true);

// Split a string into two parts at the first occurrence of delimiter
std::pair<String, String> splitFirst(const String& input, char delimiter = ':', bool trimTokens = true);

// Base64-encode a buffer and return as String
String base64EncodeBuffer(const uint8_t* buf, size_t len); 