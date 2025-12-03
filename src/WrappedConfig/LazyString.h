#pragma once

#include <soc/soc.h>
#include <cstring>
#include <string>

#include <utility>

#include <Arduino.h>

namespace WrappedConfig {
  class LazyString {
    public:
      LazyString() : LazyString("") {}
      LazyString(size_t length) { // NOLINT
        _buffer = new char[length + 1];
        _buffer[0] = '\0';
      }

      LazyString(const char* value) {
        if (value) {
          if (_isFlashString(value)) {
            _buffer = const_cast<char*>(value);

          } else {
            size_t len = strlen(value);
            char* buf = new char[len + 1];
            strcpy(buf, value);
            _buffer = buf;
          }
        }
      }

      explicit LazyString(const std::string_view& view) : LazyString(view.data()) {}
      explicit LazyString(const std::string& value) : LazyString(value.c_str()) {}

      LazyString(std::string&& value) : LazyString(value.c_str()) {
        value.clear();
      }

      LazyString(const String& value) : LazyString(value.c_str()) {}

      LazyString(String&& value) : LazyString(value.c_str()) {
        value.clear();
      }

      LazyString(const LazyString& other) {
        if (other._buffer) {
          if (_isFlashString(other._buffer)) {
            _buffer = other._buffer;

          } else {
            size_t len = strlen(other._buffer);
            char* buf = new char[len + 1];
            strcpy(buf, other._buffer);
            _buffer = buf;
          }
        }
      }

      LazyString(LazyString&& other) noexcept : _buffer(other._buffer) {
        other._buffer = nullptr;
      }

      ~LazyString() {
        if (!_isFlashString(_buffer)) {
          delete[] _buffer;
        }
      }

      LazyString& operator=(const LazyString& other) noexcept {
        if (this != &other) {
          if (!_isFlashString(_buffer)) {
            delete[] _buffer;
          }

          _buffer = nullptr;
          if (other._buffer) {
            if (_isFlashString(_buffer)) {
              _buffer = other._buffer;

            } else {
              size_t len = strlen(other._buffer);
              char* buf = new char[len + 1];
              strcpy(buf, other._buffer);
              _buffer = buf;
            }
          }
        }

        return *this;
      }

      LazyString& operator=(LazyString&& other) noexcept {
        if (this != &other) {
          if (!_isFlashString(_buffer)) {
            delete[] _buffer;
          }

          _buffer = other._buffer;
          other._buffer = nullptr;
        }

        return *this;
      }

      char* buffer() const {
        return _buffer;
      }

      const char* c_str() const {
        return _buffer;
      }

      size_t size() const {
        return _buffer ? strlen(_buffer) : 0;
      }

      size_t length() const {
        return size();
      }

      bool empty() const {
        return !_buffer || *_buffer == '\0';
      }

      // LazyString vs LazyString
      //
      friend bool operator==(const LazyString& lhs, const LazyString& rhs) {
        const char* l = lhs._buffer ? lhs._buffer : "";
        const char* r = rhs._buffer ? rhs._buffer : "";
        return l == r || strcmp(l, r) == 0;
      }

      friend bool operator!=(const LazyString& lhs, const LazyString& rhs) {
        return !(lhs == rhs);
      }

      friend bool operator<(const LazyString& lhs, const LazyString& rhs) {
        const char* l = lhs._buffer ? lhs._buffer : "";
        const char* r = rhs._buffer ? rhs._buffer : "";
        return strcmp(l, r) < 0;
      }

      friend bool operator>(const LazyString& lhs, const LazyString& rhs) {
        return rhs < lhs;
      }

      friend bool operator<=(const LazyString& lhs, const LazyString& rhs) {
        return !(rhs < lhs);
      }

      friend bool operator>=(const LazyString& lhs, const LazyString& rhs) {
        return !(lhs < rhs);
      }

      // LazyString vs const char*
      //
      friend bool operator==(const LazyString& lhs, const char* rhs) {
        const char* l = lhs._buffer ? lhs._buffer : "";
        const char* r = rhs ? rhs : "";
        return l == r || strcmp(l, r) == 0;
      }

      friend bool operator!=(const LazyString& lhs, const char* rhs) {
        return !(lhs == rhs);
      }

      friend bool operator<(const LazyString& lhs, const char* rhs) {
        const char* l = lhs._buffer ? lhs._buffer : "";
        const char* r = rhs ? rhs : "";
        return strcmp(l, r) < 0;
      }

      friend bool operator>(const LazyString& lhs, const char* rhs) {
        return rhs < lhs;
      }

      friend bool operator<=(const LazyString& lhs, const char* rhs) {
        return !(rhs < lhs);
      }

      friend bool operator>=(const LazyString& lhs, const char* rhs) {
        return !(lhs < rhs);
      }

      // const char* vs LazyString
      //
      friend bool operator==(const char* lhs, const LazyString& rhs) {
        return rhs == lhs;
      }

      friend bool operator!=(const char* lhs, const LazyString& rhs) {
        return !(lhs == rhs);
      }

      friend bool operator<(const char* lhs, const LazyString& rhs) {
        const char* l = lhs ? lhs : "";
        const char* r = rhs._buffer ? rhs._buffer : "";
        return strcmp(l, r) < 0;
      }

      friend bool operator>(const char* lhs, const LazyString& rhs) {
        return rhs < lhs;
      }

      friend bool operator<=(const char* lhs, const LazyString& rhs) {
        return !(rhs < lhs);
      }

      friend bool operator>=(const char* lhs, const LazyString& rhs) {
        return !(lhs < rhs);
      }

      // LazyString vs std::string
      //
      friend bool operator==(const LazyString& lhs, const std::string& rhs) {
        return lhs == rhs.c_str();
      }

      friend bool operator!=(const LazyString& lhs, const std::string& rhs) {
        return !(lhs == rhs);
      }

      friend bool operator<(const LazyString& lhs, const std::string& rhs) {
        return lhs < rhs.c_str();
      }

      friend bool operator>(const LazyString& lhs, const std::string& rhs) {
        return rhs < lhs;
      }

      friend bool operator<=(const LazyString& lhs, const std::string& rhs) {
        return !(rhs < lhs);
      }

      friend bool operator>=(const LazyString& lhs, const std::string& rhs) {
        return !(lhs < rhs);
      }

      // std::string vs LazyString
      //
      friend bool operator==(const std::string& lhs, const LazyString& rhs) {
        return rhs == lhs;
      }

      friend bool operator!=(const std::string& lhs, const LazyString& rhs) {
        return !(lhs == rhs);
      }

      friend bool operator<(const std::string& lhs, const LazyString& rhs) {
        return lhs.c_str() < rhs;
      }

      friend bool operator>(const std::string& lhs, const LazyString& rhs) {
        return rhs < lhs;
      }

      friend bool operator<=(const std::string& lhs, const LazyString& rhs) {
        return !(rhs < lhs);
      }

      friend bool operator>=(const std::string& lhs, const LazyString& rhs) {
        return !(lhs < rhs);
      }

      // LazyString vs Arduino String
      //
      friend bool operator==(const LazyString& lhs, const String& rhs) {
        return lhs == rhs.c_str();
      }

      friend bool operator!=(const LazyString& lhs, const String& rhs) {
        return !(lhs == rhs);
      }

      friend bool operator<(const LazyString& lhs, const String& rhs) {
        return lhs < rhs.c_str();
      }

      friend bool operator>(const LazyString& lhs, const String& rhs) {
        return rhs < lhs;
      }

      friend bool operator<=(const LazyString& lhs, const String& rhs) {
        return !(rhs < lhs);
      }

      friend bool operator>=(const LazyString& lhs, const String& rhs) {
        return !(lhs < rhs);
      }

      // Arduino String vs LazyString
      //
      friend bool operator==(const String& lhs, const LazyString& rhs) {
        return rhs == lhs;
      }

      friend bool operator!=(const String& lhs, const LazyString& rhs) {
        return !(lhs == rhs);
      }

      friend bool operator<(const String& lhs, const LazyString& rhs) {
        return lhs.c_str() < rhs;
      }

      friend bool operator>(const String& lhs, const LazyString& rhs) {
        return rhs < lhs;
      }

      friend bool operator<=(const String& lhs, const LazyString& rhs) {
        return !(rhs < lhs);
      }

      friend bool operator>=(const String& lhs, const LazyString& rhs) {
        return !(lhs < rhs);
      }

    private:
      char* _buffer = nullptr;

      __attribute__((always_inline)) inline static bool _isFlashString(const char* str) {
        return _my_esp_ptr_in_drom(str) || _my_esp_ptr_in_rom(str);
      }

      // Copy of ESP-IDF esp_ptr_in_drom in #include <esp_memory_utils.h>
      __attribute__((always_inline)) inline static bool _my_esp_ptr_in_rom(const void* p) {
        intptr_t ip = (intptr_t)p;
        return
#if CONFIG_IDF_TARGET_ARCH_RISCV && SOC_DROM_MASK_LOW != SOC_IROM_MASK_LOW
          (ip >= SOC_DROM_MASK_LOW && ip < SOC_DROM_MASK_HIGH) ||
#endif
          (ip >= SOC_IROM_MASK_LOW && ip < SOC_IROM_MASK_HIGH);
      }

      // Copy of ESP-IDF esp_ptr_in_rom in #include <esp_memory_utils.h>
      __attribute__((always_inline)) inline static bool _my_esp_ptr_in_drom(const void* p) {
        int32_t drom_start_addr = SOC_DROM_LOW;
#if CONFIG_ESP32S3_DATA_CACHE_16KB
        drom_start_addr += 0x4000;
#endif

        return ((intptr_t)p >= drom_start_addr && (intptr_t)p < SOC_DROM_HIGH);
      }
    };
}
