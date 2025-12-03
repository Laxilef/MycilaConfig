#pragma once

#include <cstring>
#include <string_view>

namespace WrappedConfig {
  static std::string_view parseKey(const char* buffer) {
    size_t length = strlen(buffer);
    const char* bufferStart = buffer;
    const char* bufferEnd = buffer + length;

    if (length > 0 && buffer[length - 1] == '\r') {
      bufferEnd--;
    }

    const char* separatorPos = static_cast<const char*>(memchr(bufferStart, '=', bufferEnd - bufferStart));
    if (separatorPos == nullptr || separatorPos == bufferStart) {
      return {};
    }

    const char* keyStart = bufferStart;
    while (keyStart < separatorPos && (*keyStart == ' ' || *keyStart == '\t' || *keyStart == '\r')) {
      keyStart++;
    }

    const char* keyEnd = separatorPos;
    while (keyEnd > keyStart && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t' || keyEnd[-1] == '\r')) {
      keyEnd--;
    }

    if (keyEnd <= keyStart) {
      return {};
    }

    return std::string_view(keyStart, keyEnd - keyStart);
  }

  static std::string_view parseValue(const char* buffer) {
    size_t length = strlen(buffer);
    const char* bufferStart = buffer;
    const char* bufferEnd = buffer + length;

    if (length > 0 && buffer[length - 1] == '\r') {
      bufferEnd--;
    }

    const char* separatorPos = static_cast<const char*>(memchr(bufferStart, '=', bufferEnd - bufferStart));
    if (separatorPos == nullptr || separatorPos == bufferStart) {
      return {};
    }

    const char* valueStart = separatorPos + 1;
    while (valueStart < bufferEnd && (*valueStart == ' ' || *valueStart == '\t' || *valueStart == '\r')) {
      valueStart++;
    }

    const char* valueEnd = bufferEnd;
    while (valueEnd > valueStart && (valueEnd[-1] == ' ' || valueEnd[-1] == '\t' || valueEnd[-1] == '\r')) {
      valueEnd--;
    }

    if (valueEnd <= valueStart) {
      return {};
    }

    return std::string_view(valueStart, valueEnd - valueStart);
  }
}
