// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <set>

#include <LittleFS.h>

#include "MycilaStorage.h"

namespace Mycila {
  class LittleFSStorage : public Storage {
    public:
      LittleFSStorage(uint32_t flushDelay = 5000) : _flushDelay(flushDelay) {}

      bool begin(const char* name = "config") override {
        if (!LittleFS.begin()) {
          ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Failed to mount LittleFS");
          return false;
        }

        _filename.reserve(4 + strlen(name));
        _filename += '/';
        _filename += name;
        _filename += ".cfg";
        return true;
      }

      void setWrapper(WrappedConfig* wrapper) override {
        _wrapper = wrapper;
      }

      std::map<const char*, std::string> load() override {
        File file = LittleFS.open(_filename.c_str(), "r");
        if (!file) {
          return {};
        }

        char buf[256];
        std::map<const char*, std::string> result;
        while (file.available()) {
          size_t len = file.readBytesUntil('\n', buf, sizeof(buf) - 1);
          if (len == 0) continue;
          buf[len] = '\0';

          const char* parsedKey;
          std::string parsedVal;
          if (_parseLine(buf, parsedKey, parsedVal)) {
            const char* pKey = _wrapper ? _wrapper->keyRef(parsedKey) : nullptr;
            if (pKey) {
              result.emplace(pKey, parsedVal);
              _persisted.insert(pKey);
            }
          }
        }
        file.close();
        return result;
      }

      bool clear() override {
        _pending.clear();
        _persisted.clear();
        _dirty = false;
        return LittleFS.remove(_filename.c_str());
      }

      bool exists(const char* key) override {
        const char* pKey = _wrapper ? _wrapper->keyRef(key) : nullptr;
        if (!pKey) {
          return false;
        }

        if (_pending.count(pKey)) {
          return _wrapper->cache().count(pKey) > 0;
        }

        return _persisted.count(pKey);
      }

      bool unset(const char* key) override {
        const char* pKey = _wrapper ? _wrapper->keyRef(key) : nullptr;
        if (!pKey) {
          return false;
        }

        if (!_wrapper->cache().count(pKey)) {
          return false;
        }

        _pending.insert(pKey);
        _dirty = true;
        _lastChange = millis();
        return true;
      }

      bool set(const char* key, const ValueVariant& value) override {
        const char* pKey = _wrapper ? _wrapper->keyRef(key) : nullptr;
        if (!pKey) {
          return false;
        }

        _pending.insert(pKey);
        _dirty = true;
        _lastChange = millis();
        return true;
      }

      ValueVariant get(const char* key, const ValueVariant& defaultValue) const override {
        const char* pKey = _wrapper ? _wrapper->keyRef(key) : nullptr;
        if (!pKey) {
          return emptyVariant;
        }

        if (_pending.count(pKey)) {
          auto it = _wrapper->cache().find(pKey);
          if (it == _wrapper->cache().end()) {
            return emptyVariant;
          }

          return it->second;
        }

        File file = LittleFS.open(_filename.c_str(), "r");
        if (!file) {
          return emptyVariant;
        }

        char buf[256];
        while (file.available()) {
          size_t length = file.readBytesUntil('\n', buf, sizeof(buf) - 1);
          if (length == 0) {
            continue;
          }
          buf[length] = '\0';

          const char* parsedKey;
          std::string parsedVal;
          if (_parseLine(buf, parsedKey, parsedVal) && strcmp(parsedKey, key) == 0) {
            file.close();
            return WrappedConfig::toVariant(parsedVal, defaultValue);
          }
        }
        file.close();
        return emptyVariant;
      }

      bool flush() {
        if (!_dirty || millis() - _lastChange < _flushDelay) {
          return false;
        }

        for (const char* pKey : _pending) {
          if (_wrapper->cache().count(pKey)) {
            _persisted.insert(pKey);
          } else {
            _persisted.erase(pKey);
          }
        }
        _pending.clear();

        std::string tmp = _filename + ".tmp";
        File out = LittleFS.open(tmp.c_str(), "w");
        if (!out) {
          ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Failed to open temp file");
          return false;
        }

        for (const char* pKey : _persisted) {
          auto it = _wrapper->cache().find(pKey);
          if (it != _wrapper->cache().end()) {
            std::string val = WrappedConfig::toString(it->second);
            out.printf("%s=%s\n", pKey, val.c_str());
          }
        }

        out.close();
        LittleFS.remove(_filename.c_str());
        if (!LittleFS.rename(tmp.c_str(), _filename.c_str())) {
          ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Failed to rename temp file");
        }
        _dirty = false;

        return true;
      }

    private:
      bool _parseLine(char* buf, const char*& outKey, std::string& outVal) const {
        if (buf[strlen(buf) - 1] == '\r') buf[strlen(buf) - 1] = '\0';

        char* eq = strchr(buf, '=');
        if (!eq || eq == buf) return false;

        *eq = '\0';
        char* keyPtr = buf;
        char* valPtr = eq + 1;

        // trim key
        while (*keyPtr == ' ' || *keyPtr == '\t') ++keyPtr;

        // trim val
        while (*valPtr == ' ' || *valPtr == '\t') ++valPtr;
        char* end = valPtr + strlen(valPtr);
        while (end > valPtr && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) --end;
        *end = '\0';

        if (*keyPtr == '\0' || *valPtr == '\0') return false;

        outKey = keyPtr;
        outVal = valPtr;
        return true;
      }

      std::string _filename;
      WrappedConfig* _wrapper = nullptr;
      std::set<const char*> _pending;
      std::set<const char*> _persisted;
      bool _dirty = false;
      unsigned long _lastChange = 0;
      unsigned long _flushDelay = 0;
  };
} // namespace Mycila
