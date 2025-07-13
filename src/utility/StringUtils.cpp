#include "StringUtils.h"

std::vector<String> splitString(const String& input, char delimiter, bool trimTokens) {
    std::vector<String> tokens;
    int start = 0;
    int end = input.indexOf(delimiter);
    while (end != -1) {
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

std::pair<String, String> splitFirst(const String& input, char delimiter, bool trimTokens) {
    int idx = input.indexOf(delimiter);
    if (idx == -1) {
        String left = input;
        if (trimTokens) left.trim();
        return {left, ""};
    }
    String left = input.substring(0, idx);
    String right = input.substring(idx + 1);
    if (trimTokens) {
        left.trim();
        right.trim();
    }
    return {left, right};
} 