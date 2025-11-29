#include "StringUtils.h"
#include <base64.hpp>

std::vector<String> splitString(const String &input, char delimiter, bool trimTokens)
{
    std::vector<String> tokens;
    int start = 0;
    int end = input.indexOf(delimiter);
    while (end != -1)
    {
        String token = input.substring(start, end);
        if (trimTokens) token.trim();
        tokens.push_back(token);
        start = end + 1;
        end = input.indexOf(delimiter, start);
    }
    String lastToken = input.substring(start);
    if (trimTokens) lastToken.trim();
    if (lastToken.length() > 0) tokens.push_back(lastToken);
    return tokens;
}

std::pair<String, String> splitFirst(const String &input, char delimiter, bool trimTokens)
{
    int idx = input.indexOf(delimiter);
    if (idx == -1)
    {
        String left = input;
        if (trimTokens) left.trim();
        return { left, "" };
    }
    String left = input.substring(0, idx);
    String right = input.substring(idx + 1);
    if (trimTokens)
    {
        left.trim();
        right.trim();
    }
    return { left, right };
}

String base64EncodeBuffer(const uint8_t *buf, size_t len)
{
    unsigned int encodedLen = encode_base64_length(len);
    unsigned char *encoded = new unsigned char[encodedLen + 1]; // +1 for null terminator
    unsigned int actualLen = encode_base64(buf, len, encoded);
    encoded[actualLen] = '\0';
    String result((char *) encoded);
    delete[] encoded;
    return result;
}

String base64DecodeString(const String &input)
{
    unsigned int decodedLen = decode_base64_length((unsigned char *) input.c_str());
    unsigned char *decoded = new unsigned char[decodedLen + 1]; // +1 for null terminator
    unsigned int actualLen = decode_base64((unsigned char *) input.c_str(), decoded);
    decoded[actualLen] = '\0';
    String result((char *) decoded);
    delete[] decoded;
    return result;
}

// Starts with 0x, so like 0x121212
uint8_t hexToUint8(const String &input)
{
    if (input.startsWith("0x"))
    {
        return strtoul(input.c_str() + 2, nullptr, 16);
    }
    return 0;
}
