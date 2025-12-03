#pragma once

#include <set>
#include <string>

#include <esp_log.h>
#include <FS.h>

#include "Base.h"
#include "../Utils.h"

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

        void setWrapper(Config* wrapper) override {
          _wrapper = wrapper;
        }

        size_t preload() override {
          auto file = _fs->open(_filename.c_str(), "r");
          if (!file) {
            return 0;
          }

          char buffer[256];
          while (file.available()) {
            size_t length = file.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
            if (length == 0) {
              continue;
            }

            buffer[length] = '\0';
            auto parsedKey = parseKey(buffer);
            if (parsedKey.empty()) {
              continue;
            }

            auto parsedKeyStr = std::string{parsedKey};
            auto* pItem = _wrapper->getItem(parsedKeyStr.c_str());
            parsedKeyStr.clear();

            if (pItem == nullptr) {
              continue;
            }

            auto parsedValueStr = std::string{parseValue(buffer)};
            auto variant = std::visit([&](auto&& def) -> Value {
              using T = std::decay_t<decltype(def)>;
              return Value::fromString<T>(parsedValueStr.c_str());
            }, pItem->getDefaultValue());
            parsedValueStr.clear();

            if (variant.isNull()) {
              continue;
            }

            pItem->setValue(variant);
            _persisted.insert(pItem->getKey());
          }
          file.close();

          return _persisted.size();
        }

        bool clear() override {
          _pending.clear();
          _persisted.clear();
          _dirty = false;
          return _fs->remove(_filename.c_str());
        }

        bool exists(const char* key) const override {
          auto* pItem = _wrapper->getItem(key);
          if (pItem == nullptr) {
            return false;
          }

          if (_pending.count(pItem->getKey())) {
            return pItem->hasValue();
          }

          return _persisted.count(pItem->getKey());
        }

        bool unset(const char* key) override {
          auto* pItem = _wrapper->getItem(key);
          if (pItem == nullptr) {
            return false;
          }

          if (!pItem->hasValue()) {
            return false;
          }

          _pending.insert(pItem->getKey());
          _dirty = true;
          _lastChange = millis();
          return true;
        }

        bool set(const char* key, const Value& value) override {
          auto* pItem = _wrapper->getItem(key);
          if (pItem == nullptr) {
            return false;
          }

          _pending.insert(pItem->getKey());
          _dirty = true;
          _lastChange = millis();
          return true;
        }

        Value get(const char* key, const Value& defaultValue) const override {
          auto* pItem = _wrapper->getItem(key);
          if (pItem == nullptr) {
            return Value::null();
          }

          if (_pending.count(pItem->getKey())) {
            if (!pItem->hasValue()) {
              return Value::null();
            }

            return pItem->getValue();
          }

          auto file = _fs->open(_filename.c_str(), "r");
          if (!file) {
            return Value::null();
          }

          char buffer[256];
          while (file.available()) {
            size_t length = file.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
            if (length == 0) {
              continue;
            }

            buffer[length] = '\0';
            auto parsedKey = parseKey(buffer);
            if (parsedKey.empty() || parsedKey != pItem->getKey()) {
              continue;
            }

            const auto parsedValueStr = std::string{parseValue(buffer)};
            file.close();

            return std::visit([&](auto&& def) -> Value {
              using T = std::decay_t<decltype(def)>;
              return Value::fromString<T>(parsedValueStr.c_str());
            }, pItem->getDefaultValue());
          }

          file.close();
          return Value::null();
        }

        bool flush() {
          if (!_dirty || millis() - _lastChange < _flushDelay) {
            return false;
          }

          for (const char* key : _pending) {
            auto* pItem = _wrapper->getItem(key);

            if (pItem && pItem->hasValue()) {
              _persisted.insert(pItem->getKey());

            } else {
              _persisted.erase(pItem->getKey());
            }
          }
          _pending.clear();

          std::string tmp = _filename + ".tmp";
          auto out = _fs->open(tmp.c_str(), "w");
          if (!out) {
            ESP_LOGE(WRAPPED_CONFIG_LOG_TAG, "Failed to open temp file");
            return false;
          }

          for (const char* key : _persisted) {
            auto* pItem = _wrapper->getItem(key);
            if (pItem && pItem->hasValue()) {
              // key
              out.print(pItem->getKey());
              out.print('=');
              
              // value
              out.print(pItem->getValue().as<const char*>());
              out.print('\n');
            }
          }

          out.close();
          _fs->remove(_filename.c_str());
          if (!_fs->rename(tmp.c_str(), _filename.c_str())) {
            ESP_LOGE(WRAPPED_CONFIG_LOG_TAG, "Failed to rename temp file");
          }
          _dirty = false;

          return true;
        }

      private:
        ::FS* _fs = nullptr;
        std::string _filename;
        Config* _wrapper = nullptr;
        std::set<const char*> _pending;
        std::set<const char*> _persisted;
        bool _dirty = false;
        unsigned long _lastChange = 0;
        unsigned long _flushDelay = 0;
    };
  }
}
