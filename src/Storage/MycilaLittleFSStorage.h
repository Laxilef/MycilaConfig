// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <LittleFS.h>

#include "MycilaConfig.h"
#include "MycilaStorage.h"

namespace Mycila {
  class LittleFSStorage : public Storage {
    public:
      bool begin(const char* name = "config") override {
        if (!LittleFS.begin()) {
          ESP_LOGE(TAG, "Failed to mount LittleFS");
          return false;
        }

        _filename.reserve(4 + strlen(name));
        _filename += '/';
        _filename += name;
        _filename += ".cfg";
        return true;
      }

      std::map<const char*, std::string> load(const std::vector<const char*>& keys) override {
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
          if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';
          char* eq = strchr(buf, '=');
          if (!eq || eq == buf) continue;
          *eq = '\0';
          char* keyPtr = buf;
          char* valPtr = eq + 1;
          while (*keyPtr == ' ' || *keyPtr == '\t') ++keyPtr;
          while (*valPtr == ' ' || *valPtr == '\t') ++valPtr;
          char* end = valPtr + strlen(valPtr);
          while (end > valPtr && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) --end;
          *end = '\0';
          if (*keyPtr == '\0' || *valPtr == '\0') continue;

          auto it = std::lower_bound(
            keys.begin(), keys.end(), keyPtr,
            [](const char* a, const char* b) {
              return strcmp(a, b) < 0;
            }
          );

          if (it != keys.end() && strcmp(*it, keyPtr) == 0) {
            result.emplace(*it, valPtr);
          }
        }
        file.close();
        return result;
      }

      bool clear() override {
        return LittleFS.remove(_filename.c_str());
      }

      bool exists(const char* key) override {
        File file = LittleFS.open(_filename.c_str(), "r");
        if (!file) return false;
        std::string prefix = std::string(key) + "=";
        char buf[256];
        while (file.available()) {
          size_t len = file.readBytesUntil('\n', buf, sizeof(buf) - 1);
          if (len == 0) continue;
          buf[len] = '\0';
          if (strstr(buf, prefix.c_str()) == buf) {
            file.close();
            return true;
          }
        }
        file.close();
        return false;
      }

      bool unset(const char* key) override {
        File in = LittleFS.open(_filename.c_str(), "r");
        if (!in) return false;
        std::vector<std::string> lines;
        bool found = false;
        std::string prefix = std::string(key) + "=";
        char buf[256];
        while (in.available()) {
          size_t len = in.readBytesUntil('\n', buf, sizeof(buf) - 1);
          if (len == 0) continue;
          buf[len] = '\0';
          std::string line(buf);
          line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
          if (line.find(prefix) == 0) found = true;
          else if (!line.empty()) lines.push_back(std::move(line));
        }
        in.close();
        if (!found) return false;
        return _writeLines(lines);
      }

      bool set(const char* key, const ValueVariant& value) override {
        std::string val = WrappedConfig::toString(value);
        if (val.empty()) return unset(key);
        File in = LittleFS.open(_filename.c_str(), "r");
        std::vector<std::string> lines;
        bool found = false;
        std::string prefix = std::string(key) + "=";
        std::string newLine = prefix + val;
        if (in) {
          char buf[256];
          while (in.available()) {
            size_t len = in.readBytesUntil('\n', buf, sizeof(buf) - 1);
            if (len == 0) continue;
            buf[len] = '\0';
            std::string line(buf);
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            if (line.find(prefix) == 0) {
              lines.push_back(newLine);
              found = true;
            } else if (!line.empty()) lines.push_back(std::move(line));
          }
          in.close();
        }
        if (!found) lines.push_back(newLine);
        return _writeLines(lines);
      }

      ValueVariant get(const char* key, const ValueVariant& def) const override {
        return def;  // assume loaded to cache, file unchanged
      }

    private:
      bool _writeLines(const std::vector<std::string>& lines) {
        if (lines.empty()) return LittleFS.remove(_filename.c_str());
        std::string tmp = _filename + ".tmp";
        File out = LittleFS.open(tmp.c_str(), "w");
        if (!out) {
          ESP_LOGE(TAG, "Failed to open temp file");
          return false;
        }
        for (const auto& l : lines) if (!l.empty()) out.println(l.c_str());
        out.close();
        LittleFS.remove(_filename.c_str());
        return LittleFS.rename(tmp.c_str(), _filename.c_str());
      }

      std::string _filename;
  };
} // namespace Mycila
