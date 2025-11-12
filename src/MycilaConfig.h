// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <Print.h>

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#include "MycilaConfigTypes.h"
#include "Storage/MycilaStorage.h"
#include "Storage/MycilaPreferencesStorage.h"

#define MYCILA_CONFIG_VERSION          "8.0.1"
#define MYCILA_CONFIG_VERSION_MAJOR    8
#define MYCILA_CONFIG_VERSION_MINOR    0
#define MYCILA_CONFIG_VERSION_REVISION 1
#define TAG                            "CONFIG"

// suffix to use for a setting key enabling a feature
#ifndef MYCILA_CONFIG_KEY_ENABLE_SUFFIX
  #define MYCILA_CONFIG_KEY_ENABLE_SUFFIX "_enable"
#endif

// suffix to use for a setting key representing a password
#ifndef MYCILA_CONFIG_KEY_PASSWORD_SUFFIX
  #define MYCILA_CONFIG_KEY_PASSWORD_SUFFIX "_pwd"
#endif

#ifndef MYCILA_CONFIG_SHOW_PASSWORD
  #ifndef MYCILA_CONFIG_PASSWORD_MASK
    #define MYCILA_CONFIG_PASSWORD_MASK "********"
  #endif
#endif

namespace Mycila {
  class WrappedConfig {
    public:
      enum class Result {
        PERSISTED,
        UNKNOWN_KEY,
        ALREADY_PERSISTED,
        SAME_AS_DEFAULT,
        INVALID_VALUE,
        TYPE_MISMATCH,
        FAIL_ON_WRITE
      };

      class SetResult {
        public:
          constexpr SetResult(Result result) noexcept : _result(result) {} // NOLINT
          constexpr operator bool() const { return _result == Result::PERSISTED; }
          constexpr operator Result() const { return _result; }

        private:
          Result _result;
      };

      WrappedConfig(std::shared_ptr<Storage> storage) : _storage(std::move(storage)) {}
      ~WrappedConfig() = default;

      // Add a new configuration key with its default value
      void configure(const char* key, ValueVariant defaultValue = std::string{});

      // starts the config system
      void begin(const char* name = "CONFIG");

      // register a callback to be called when a config value changes
      void listen(ConfigChangeCallback callback) { _changeCallback = callback; }

      // register a callback to be called when the configuration is restored
      void listen(ConfigRestoredCallback callback) { _restoreCallback = callback; }

      // register a global callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(ConfigValidatorCallback callback);

      // register a callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(const char* key, ConfigValidatorCallback callback);

      // returns false if the key is not found
      bool exists(const char* key) const { return std::find(_keys.begin(), _keys.end(), key) != _keys.end(); };

      // get the optional value variant of a setting key
      std::optional<const Mycila::ValueVariant> get(const char* key) const;

      // get a pointer to a string or default of a setting key
      const char* get(const char* key, const char* defaultValue) const;

      // get the value or default of a setting key
      template <typename T>
      const T& get(const char* key, const T& defaultValue = T{}) const {
        // check if we have a cached value
        auto it = _cache.find(key);
        if (it != _cache.end() && std::holds_alternative<T>(it->second)) {
          return std::get<T>(it->second);
        }

        // not in cache ? is it a real key ?
        if (!exists(key)) {
          ESP_LOGW(TAG, "get(%s): Key unknown", key);
          return defaultValue;
        }

        // real key exists ?
        if (_storage->exists(key)) {
          _cache[key] = _storage->get(key, _defaults.at(key));
          ESP_LOGD(TAG, "get(%s): Key cached", key);

          if (std::holds_alternative<T>(_cache[key])) {
            return std::get<T>(_cache[key]);
          }
        }

        // key does not exist, or not assigned to a value
        _cache[key] = _defaults.at(key);
        return std::get<T>(_cache[key]);
      }

      /*template <typename T>
      T get(const char* key, T defaultValue = T{}) const {
        const auto optValue = get(key);
        if (optValue.has_value() && std::holds_alternative<T>(optValue.value())) {
          return std::get<T>(optValue.value());
        }

        return defaultValue;
      }*/

      bool isEqual(const char* key, const ValueVariant& value) const;

      const SetResult set(const char* key, ValueVariant value, bool fireChangeCallback = true);
      bool set(const std::map<const char*, ValueVariant>& settings, bool fireChangeCallback = true);

      bool unset(const char* key, bool fireChangeCallback = true);

      bool isPasswordKey(const char* key) const;
      bool isEnableKey(const char* key) const;

      void backup(Print& out); // NOLINT
      bool restore(const char* data);
      bool restore(const std::map<const char*, ValueVariant>& settings);

      // clear all saved settings and current cache
      void clear();

      // get list of keys
      const std::vector<const char*>& keys() const { return _keys; }

      // this method can be used to find the right pointer to a supported key given a random buffer
      const char* keyRef(const char* buffer) const;

#ifdef MYCILA_JSON_SUPPORT
      bool toJson(JsonObject root, const char* key);
      void toJson(JsonObject root);
#endif

      static std::string toString(const ValueVariant& variant);
      static ValueVariant toVariant(const std::string& value, const ValueVariant& def);

    protected:
      std::shared_ptr<Storage> _storage;
      ConfigChangeCallback _changeCallback = nullptr;
      ConfigRestoredCallback _restoreCallback = nullptr;
      ConfigValidatorCallback _globalValidatorCallback = nullptr;
      std::vector<const char*> _keys;
      mutable std::map<const char*, ValueVariant> _defaults;
      mutable std::map<const char*, ValueVariant> _cache;
      mutable std::map<const char*, ConfigValidatorCallback> _validators;
      const ValueVariant empty = std::string{};
  };

  class Config : public WrappedConfig {
    public:
      [[deprecated]]
      Config() : WrappedConfig(std::make_shared<PreferencesStorage>()) {}
      ~Config() = default;
  };
} // namespace Mycila
