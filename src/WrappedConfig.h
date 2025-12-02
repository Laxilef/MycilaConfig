// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <Print.h>

#ifdef WRAPPED_CONFIG_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#define MYCILA_CONFIG_VERSION          "8.0.1"
#define MYCILA_CONFIG_VERSION_MAJOR    8
#define MYCILA_CONFIG_VERSION_MINOR    0
#define MYCILA_CONFIG_VERSION_REVISION 1
#define MYCILA_CONFIG_LOG_TAG          "CONFIG"

#ifndef WRAPPED_CONFIG_USE_LONG_LONG
  #define WRAPPED_CONFIG_USE_LONG_LONG 1
#endif

#ifndef WRAPPED_CONFIG_USE_DOUBLE
  #define WRAPPED_CONFIG_USE_DOUBLE 1
#endif

#ifndef WRAPPED_CONFIG_VALUE_TRUE
  #define WRAPPED_CONFIG_VALUE_TRUE "true"
#endif

#ifndef WRAPPED_CONFIG_VALUE_FALSE
  #define WRAPPED_CONFIG_VALUE_FALSE "false"
#endif

#ifndef WRAPPED_CONFIG_EXTENDED_BOOL_VALUE_PARSING
  #define WRAPPED_CONFIG_EXTENDED_BOOL_VALUE_PARSING 1
#endif

// suffix to use for a setting key enabling a feature
#ifndef WRAPPED_CONFIG_KEY_ENABLED_SUFFIX
  #define WRAPPED_CONFIG_KEY_ENABLED_SUFFIX "_enable"
#endif

// suffix to use for a setting key representing a password
#ifndef WRAPPED_CONFIG_KEY_PASSWORD_SUFFIX
  #define WRAPPED_CONFIG_KEY_PASSWORD_SUFFIX "_pwd"
#endif

#ifndef WRAPPED_CONFIG_SHOW_PASSWORD
  #ifndef WRAPPED_CONFIG_PASSWORD_MASK
    #define WRAPPED_CONFIG_PASSWORD_MASK "********"
  #endif
#endif

#include "Types.h"
#include "Storage/Base.h"

namespace WrappedConfig {
  class WrappedConfig {
    public:
      WrappedConfig(std::shared_ptr<Storage::Base> storage, uint16_t reserve = 10) : _storage(std::move(storage)) {
        _items.reserve(reserve);
      }

      virtual ~WrappedConfig() {
        flush();
      };

      // Add a new configuration key with its default value
      template <typename T = Value>
      WrappedConfig& configure(const char* key, T defaultValue = T{}) {
        assert(!_began);

        const Item& item = _items.emplace_back(key, std::move(defaultValue));
        ESP_LOGD(
          MYCILA_CONFIG_LOG_TAG, "Config Key '%s' defaults to '%s'",
          key, item.getDefaultValue().as<const char*>()
        );

        return *this;
      }

      // starts the config system
      WrappedConfig& begin(const char* name = "config");

      // Write config if necessary
      bool flush() {
        return _storage->flush();
      }

      // register a callback to be called when a config value changes
      WrappedConfig& listen(ConfigChangeCallback callback) {
        _changeCallback = std::move(callback);
        return *this;
      }

      // register a callback to be called when the configuration is restored
      WrappedConfig& listen(ConfigRestoredCallback callback) {
        _restoreCallback = std::move(callback);
        return *this;
      }

      // register a global callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(ConfigValidatorCallback callback);

      // register a callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(const char* key, ConfigValidatorCallback callback);

      // returns false if the key is not found
      bool exists(const char* key) const;

      Item* getItem(const char* key) const;

      // get the value variant of a setting key
      const Value& get(const char* key);

      // get the value or default of a setting key
      template <typename T>
      T get(const char* key, T defaultValue = T{}) {
        const auto& variant = get(key);
        if (variant.isNull()) {
          return defaultValue;
        }

        return variant.as<T>();
      }

      bool isEqual(const char* key, const Value& value);

      const Result set(const char* key, Value value, bool fireChangeCallback = true);
      bool set(const std::map<const char*, Value>& settings, bool fireChangeCallback = true);

      const Result unset(const char* key, bool fireChangeCallback = true);

      void backup(Print& out); // NOLINT
      bool restore(const char* data);
      bool restore(const std::map<const char*, Value>& settings);

      // clear all saved settings and current cache
      void clear();

      // get list of items
      const auto& items() const {
        return _items;
      }

#ifdef WRAPPED_CONFIG_JSON_SUPPORT
      bool toJson(JsonObject root, const char* key);
      void toJson(JsonObject root);
#endif

    protected:
      bool _began = false;
      std::shared_ptr<Storage::Base> _storage;
      ConfigChangeCallback _changeCallback = nullptr;
      ConfigRestoredCallback _restoreCallback = nullptr;
      ConfigValidatorCallback _globalValidatorCallback = nullptr;
      mutable std::vector<Item> _items;
      mutable std::vector<ValidatorPair> _validators;
  };
} // namespace WrappedConfig
