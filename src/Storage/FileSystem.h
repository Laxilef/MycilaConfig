#pragma once

#include <set>

#include <FS.h>

#include "Base.h"

namespace WrappedConfig {
  namespace Storage {
    class FileSystem : public Base {
      public:
        FileSystem(::FS* fs, uint32_t flushDelay = 5000) : _fs(fs), _flushDelay(flushDelay) {}

        bool begin(const char* name) override {
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
          auto file = _fs->open(_filename.c_str(), "r");
          if (!file) {
            return {};
          }

          char buf[256];
          std::map<const char*, std::string> result;
          while (file.available()) {
            size_t len = file.readBytesUntil('\n', buf, sizeof(buf) - 1);
            if (len == 0) {
              continue;
            }

            buf[len] = '\0';
            const char* parsedKey;
            std::string parsedVal;
            if (_parseLine(buf, parsedKey, parsedVal)) {
              auto* item = _wrapper->getItem(parsedKey);
              
              if (item != nullptr) {
                result.emplace(item->getKey(), parsedVal);
                _persisted.insert(item->getKey());
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
          return _fs->remove(_filename.c_str());
        }

        bool exists(const char* key) override {
          auto* item = _wrapper->getItem(key);
          if (item == nullptr) {
            return false;
          }

          if (_pending.count(item->getKey())) {
            return item->hasValue();
          }

          return _persisted.count(item->getKey());
        }

        bool unset(const char* key) override {
          auto* item = _wrapper->getItem(key);
          if (item == nullptr) {
            return false;
          }

          if (!item->hasValue()) {
            return false;
          }

          _pending.insert(item->getKey());
          _dirty = true;
          _lastChange = millis();
          return true;
        }

        bool set(const char* key, const Value& value) override {
          auto* item = _wrapper->getItem(key);
          if (item == nullptr) {
            return false;
          }

          _pending.insert(item->getKey());
          _dirty = true;
          _lastChange = millis();
          return true;
        }

        Value get(const char* key, const Value& defaultValue) const override {
          auto* item = _wrapper->getItem(key);
          if (item == nullptr) {
            return Value::null();
          }

          if (_pending.count(item->getKey())) {
            if (!item->hasValue()) {
              return Value::null();
            }

            return item->getValue();
          }

          auto file = _fs->open(_filename.c_str(), "r");
          if (!file) {
            return Value::null();
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
            if (_parseLine(buf, parsedKey, parsedVal) && strcmp(parsedKey, item->getKey()) == 0) {
              file.close();
              
              return std::visit([&](auto&& def) -> Value {
                using T = std::decay_t<decltype(def)>;
                return Value::fromString<T>(parsedVal.c_str());
              }, item->getDefaultValue());
            }
          }
          file.close();
          return Value::null();
        }

        bool flush() {
          if (!_dirty || millis() - _lastChange < _flushDelay) {
            return false;
          }

          for (const char* key : _pending) {
            auto* item = _wrapper->getItem(key);

            if (item && item->hasValue()) {
              _persisted.insert(item->getKey());

            } else {
              _persisted.erase(item->getKey());
            }
          }
          _pending.clear();

          std::string tmp = _filename + ".tmp";
          auto out = _fs->open(tmp.c_str(), "w");
          if (!out) {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Failed to open temp file");
            return false;
          }

          for (const char* key : _persisted) {
            auto* item = _wrapper->getItem(key);
            if (item && item->hasValue()) {
              auto value = item->getValue().toString();
              out.printf("%s=%s\n", item->getKey(), value.data());
            }
          }

          out.close();
          _fs->remove(_filename.c_str());
          if (!_fs->rename(tmp.c_str(), _filename.c_str())) {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Failed to rename temp file");
          }
          _dirty = false;

          return true;
        }

      private:
        ::FS* _fs = nullptr;
        std::string _filename;
        WrappedConfig* _wrapper = nullptr;
        std::set<const char*> _pending;
        std::set<const char*> _persisted;
        bool _dirty = false;
        unsigned long _lastChange = 0;
        unsigned long _flushDelay = 0;

        bool _parseLine(char* buf, const char*& outKey, std::string& outVal) const {
          if (buf[strlen(buf) - 1] == '\r') {
            buf[strlen(buf) - 1] = '\0';
          }

          char* eq = strchr(buf, '=');
          if (!eq || eq == buf) {
            return false;
          }

          *eq = '\0';
          char* keyPtr = buf;
          char* valPtr = eq + 1;

          // trim key
          while (*keyPtr == ' ' || *keyPtr == '\t') {
            ++keyPtr;
          }

          // trim val
          while (*valPtr == ' ' || *valPtr == '\t') {
            ++valPtr;
          }

          char* end = valPtr + strlen(valPtr);
          while (end > valPtr && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
            --end;
          }
          *end = '\0';

          if (*keyPtr == '\0' || *valPtr == '\0') {
            return false;
          }

          outKey = keyPtr;
          outVal = valPtr;
          return true;
        }
    };
  }
} // namespace WrappedConfig
