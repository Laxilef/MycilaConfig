// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <string>

#include <Arduino.h>

// Copy of ESP-IDF esp_ptr_in_drom_alt
__attribute__((always_inline)) inline static bool _my_esp_ptr_in_rom(const void* p) {
  intptr_t ip = (intptr_t)p;
  return
#if CONFIG_IDF_TARGET_ARCH_RISCV && SOC_DROM_MASK_LOW != SOC_IROM_MASK_LOW
    (ip >= SOC_DROM_MASK_LOW && ip < SOC_DROM_MASK_HIGH) ||
#endif
    (ip >= SOC_IROM_MASK_LOW && ip < SOC_IROM_MASK_HIGH);
}

// Copy of ESP-IDF esp_ptr_in_rom
__attribute__((always_inline)) inline static bool _my_esp_ptr_in_drom(const void* p) {
  int32_t drom_start_addr = SOC_DROM_LOW;
#if CONFIG_ESP32S3_DATA_CACHE_16KB
  drom_start_addr += 0x4000;
#endif

  return ((intptr_t)p >= drom_start_addr && (intptr_t)p < SOC_DROM_HIGH);
}

__attribute__((always_inline)) inline static bool isFlashString(const char* str) { return _my_esp_ptr_in_drom(str) || _my_esp_ptr_in_rom(str); }

static constexpr void deleter_noop(char[]) {}
static constexpr void deleter_delete(char p[]) { delete[] p; }

namespace Mycila {
  class LazyString {
    private:
      const char* _str = nullptr;

    public:
      LazyString() = default;

      LazyString(const char* value) {
        if (value) {
          if (isFlashString(value)) {
            _str = value;
          } else {
            size_t len = strlen(value);
            char* buf = new char[len + 1];
            strcpy(buf, value);
            _str = buf;
          }
        }
      }

      LazyString(const std::string& value) : LazyString(value.c_str()) {}

      LazyString(std::string&& value) : LazyString(value.c_str()) {
        value.clear();
      }

      LazyString(const String& value) : LazyString(value.c_str()) {}

      LazyString(String&& value) : LazyString(value.c_str()) {
        value.clear();
      }

      LazyString(const LazyString& other) {
        if (other._str) {
          if (isFlashString(other._str)) {
            _str = other._str;
          } else {
            size_t len = strlen(other._str);
            char* buf = new char[len + 1];
            strcpy(buf, other._str);
            _str = buf;
          }
        }
      }

      LazyString(LazyString&& other) noexcept : _str(other._str) {
        other._str = nullptr;
      }

      ~LazyString() {
        if (!isFlashString(_str)) {
          delete[] _str;
        }
      }

      LazyString& operator=(const LazyString& other) {
        if (this != &other) {
          if (!isFlashString(_str)) {
            delete[] _str;
          }
          _str = nullptr;
          if (other._str) {
            if (isFlashString(_str)) {
              _str = other._str;
            } else {
              size_t len = strlen(other._str);
              char* buf = new char[len + 1];
              strcpy(buf, other._str);
              _str = buf; 
            }
          }
        }
        return *this;
      }

      LazyString& operator=(LazyString&& other) noexcept {
        if (this != &other) {
          if (!isFlashString(_str)) {
            delete[] _str;
          }
          _str = other._str;
          other._str = nullptr;
        }
        return *this;
      }

      const char* c_str() const { return _str ? _str : ""; }
      size_t size() const { return _str ? strlen(_str) : 0; }
      bool empty() const { return !_str || *_str == '\0'; }

      bool operator==(const LazyString& other) const {
        return strcmp(c_str(), other.c_str()) == 0;
      }
  };
} // namespace Mycila
