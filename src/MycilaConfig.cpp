// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include "MycilaConfig.h"

#include <assert.h>

void Mycila::WrappedConfig::begin(const char* name) {
  ESP_LOGI(TAG, "Initializing Config System: %s...", name);
  _storage->begin(name);
}

bool Mycila::WrappedConfig::setValidator(ConfigValidatorCallback callback) {
  if (callback) {
    _globalValidatorCallback = callback;
    ESP_LOGD(TAG, "setValidator(callback)");
  } else {
    _globalValidatorCallback = nullptr;
    ESP_LOGD(TAG, "setValidator(nullptr)");
  }
  return true;
}

bool Mycila::WrappedConfig::setValidator(const char* key, ConfigValidatorCallback callback) {
  // check if the key is valid
  if (!exists(key)) {
    ESP_LOGW(TAG, "setValidator(%s): Unknown key!", key);
    return false;
  }

  if (callback) {
    _validators[key] = callback;
    ESP_LOGD(TAG, "setValidator(%s, callback)", key);
  } else {
    _validators.erase(key);
    ESP_LOGD(TAG, "setValidator(%s, nullptr)", key);
  }

  return true;
}

void Mycila::WrappedConfig::configure(const char* key, ValueVariant defaultValue) {
  _keys.push_back(key);
  std::sort(_keys.begin(), _keys.end(), [](const char* a, const char* b) { return strcmp(a, b) < 0; });
  _defaults[key] = std::move(defaultValue);
  ESP_LOGD(TAG, "Config Key '%s' defaults to '%s'", key, toString(_defaults[key]).c_str());
}

std::optional<const Mycila::ValueVariant> Mycila::WrappedConfig::get(const char* key) const {
  // check if we have a cached value
  auto it = _cache.find(key);
  if (it != _cache.end()) {
    return it->second;
  }

  // not in cache ? is it a real key ?
  if (!exists(key)) {
    ESP_LOGW(TAG, "get(%s): Key unknown", key);
    return std::nullopt;
  }

  // real key exists ?
  if (_storage->exists(key)) {
    _cache[key] = _storage->get(key, _defaults.at(key));
    ESP_LOGD(TAG, "get(%s): Key cached", key);
    return _cache[key];
  }

  // key does not exist, or not assigned to a value
  _cache[key] = _defaults.at(key);
  return _cache[key];
}

const char* Mycila::WrappedConfig::get(const char* key, const char* defaultValue) const {
  // check if we have a cached value
  auto it = _cache.find(key);
  if (it != _cache.end() && std::holds_alternative<std::string>(it->second)) {
    return std::get<std::string>(it->second).c_str();
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

    if (std::holds_alternative<std::string>(_cache[key])) {
      return std::get<std::string>(_cache[key]).c_str();
    }
  }

  // key does not exist, or not assigned to a value
  _cache[key] = _defaults.at(key);

  if (std::holds_alternative<std::string>(_cache[key])) {
    return std::get<std::string>(_cache[key]).c_str();
  }

  return defaultValue;
}

bool Mycila::WrappedConfig::isEqual(const char* key, const ValueVariant& value) const {
  return get(key) == value;
}

const Mycila::WrappedConfig::SetResult Mycila::WrappedConfig::set(const char* key, ValueVariant value, bool fireChangeCallback) {
  // check if the key is valid
  if (!exists(key)) {
    ESP_LOGW(TAG, "set(%s): UNKNOWN_KEY", key);
    return Result::UNKNOWN_KEY;
  }

  // check if the type valid
  if (value.index() != _defaults.at(key).index()) {
    ESP_LOGD(TAG, "set(%s): TYPE_MISMATCH", key);
    return Result::TYPE_MISMATCH;
  }


  bool keyExists = _storage->exists(key);
  auto current = keyExists ? get(key).value_or(_defaults.at(key)) : _defaults.at(key);

  // key there and set to value
  // or key not there and set to default value
  if (current == value) {
    ESP_LOGD(TAG, "set(%s): %s", key, keyExists ? "ALREADY_PERSISTED" : "SAME_AS_DEFAULT");
    return keyExists ? Result::ALREADY_PERSISTED : Result::SAME_AS_DEFAULT;
  }

  // check if we have a global validator
  // and check if the value is valid
  if (_globalValidatorCallback && !_globalValidatorCallback(key, value)) {
    ESP_LOGD(TAG, "set(%s): INVALID_VALUE", key);
    return Result::INVALID_VALUE;
  }

  // check if we have a specific validator for the key
  // and check if the value is valid
  auto it = _validators.find(key);
  if (it != _validators.end() && !it->second(key, value)) {
    ESP_LOGD(TAG, "set(%s): INVALID_VALUE", key);
    return Result::INVALID_VALUE;
  }

  // update failed ?
  if (!_storage->set(key, value)) {
    ESP_LOGD(TAG, "set(%s): FAIL_ON_WRITE", key);
    return Result::FAIL_ON_WRITE;
  }

  _cache[key] = std::move(value);
  ESP_LOGD(TAG, "set(%s): PERSISTED", key);
  if (fireChangeCallback && _changeCallback)
    _changeCallback(key, _cache[key]);

  return Result::PERSISTED;
}

bool Mycila::WrappedConfig::set(const std::map<const char*, ValueVariant>& settings, bool fireChangeCallback) {
  bool updates = false;

  // start restoring settings
  for (auto& key : _keys)
    if (!isEnableKey(key) && settings.find(key) != settings.end())
      updates |= set(key, settings.at(key), fireChangeCallback);

  // then restore settings enabling/disabling a feature
  for (auto& key : _keys)
    if (isEnableKey(key) && settings.find(key) != settings.end())
      updates |= set(key, settings.at(key), fireChangeCallback);

  return updates;
}

bool Mycila::WrappedConfig::unset(const char* key, bool fireChangeCallback) {
  // check if the key is valid
  if (!exists(key)) {
    ESP_LOGW(TAG, "unset(%s): Unknown key!", key);
    return false;
  }

  // key not there or not removed
  if (!_storage->exists(key) || !_storage->unset(key))
    return false;

  // key there and to remove
  _cache.erase(key);
  ESP_LOGD(TAG, "unset(%s)", key);
  if (fireChangeCallback && _changeCallback)
    _changeCallback(key, empty);

  return true;
}

void Mycila::WrappedConfig::backup(Print& out) {
  for (auto& key : _keys) {
    auto value = get(key).value_or(_defaults.at(key));
    out.print(key);
    out.print('=');
    out.print(toString(value).c_str());
    out.print("\n");
  }
}

bool Mycila::WrappedConfig::restore(const char* data) {
  std::map<const char*, ValueVariant> settings;
  for (auto& key : _keys) {
    // int start = data.indexOf(key);
    char* start = strstr(data, key);
    if (start) {
      start += strlen(key);
      if (*start != '=')
        continue;
      start++;
      char* end = strchr(start, '\r');
      if (!end)
        end = strchr(start, '\n');
      if (!end) {
        ESP_LOGW(TAG, "restore(%s): Invalid data!", key);
        return false;
      }

      if (exists(key)) {
        std::string val(start, end - start);
        settings[key] = toVariant(val, _defaults.at(key));
      }
    }
  }
  return restore(settings);
}

bool Mycila::WrappedConfig::restore(const std::map<const char*, ValueVariant>& settings) {
  ESP_LOGD(TAG, "Restoring %d settings...", settings.size());
  bool restored = set(settings, false);
  if (restored) {
    ESP_LOGD(TAG, "Config restored");
    if (_restoreCallback)
      _restoreCallback();
  } else
    ESP_LOGD(TAG, "No change detected");
  return restored;
}

void Mycila::WrappedConfig::clear() {
  _storage->clear();
  _cache.clear();
}

bool Mycila::WrappedConfig::isPasswordKey(const char* key) const {
  size_t len = strlen(key);
  if (len < sizeof(MYCILA_CONFIG_KEY_PASSWORD_SUFFIX) - 1)
    return false;
  return strcmp(key + len - sizeof(MYCILA_CONFIG_KEY_PASSWORD_SUFFIX) - 1, MYCILA_CONFIG_KEY_PASSWORD_SUFFIX) == 0;
}

bool Mycila::WrappedConfig::isEnableKey(const char* key) const {
  size_t len = strlen(key);
  if (len < sizeof(MYCILA_CONFIG_KEY_ENABLE_SUFFIX) - 1)
    return false;
  return strcmp(key + len - sizeof(MYCILA_CONFIG_KEY_ENABLE_SUFFIX) - 1, MYCILA_CONFIG_KEY_ENABLE_SUFFIX) == 0;
}

const char* Mycila::WrappedConfig::keyRef(const char* buffer) const {
  for (auto& k : _keys)
    if (strcmp(k, buffer) == 0)
      return k;
  return nullptr;
}

#ifdef MYCILA_JSON_SUPPORT
bool Mycila::WrappedConfig::toJson(JsonObject root, const char* key) {
  const auto optionalValue = get(key);
  if (!optionalValue.has_value()) {
    return false;
  }

  return std::visit([&](auto&& value) -> bool { 
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same_v<T, bool> || std::is_arithmetic_v<T>) {
      root[key] = value;
      return true;
    
    } else if constexpr (std::is_same_v<T, std::string>) {
#ifdef MYCILA_CONFIG_PASSWORD_MASK
      root[key] = !isPasswordKey(key) ? value : MYCILA_CONFIG_PASSWORD_MASK;
#else
      root[key] = value;
#endif
      return true;
    }

    return false;
  }, optionalValue.value());
}

void Mycila::WrappedConfig::toJson(JsonObject root) {
  for (auto& key : _keys) {
    toJson(root, key);
  }
}
#endif

std::string Mycila::WrappedConfig::toString(const Mycila::ValueVariant& variant) {
  return std::visit([](auto&& value) -> std::string { 
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same_v<T, bool>)              return value ? "true" : "false";
    else if constexpr (std::is_arithmetic_v<T>)         return std::to_string(value);
    else if constexpr (std::is_same_v<T, std::string>)  return value;
    return std::string{};
  }, variant);
}

Mycila::ValueVariant Mycila::WrappedConfig::toVariant(const std::string& value, const Mycila::ValueVariant& def) {
  return std::visit([&](auto&& variant) -> ValueVariant { 
    using T = std::decay_t<decltype(variant)>;

    if constexpr (std::is_same_v<T, bool>) {
      if (value.compare("true") == 0 || value.compare("1") == 0 || value.compare("on") == 0 || value.compare("yes") == 0) {
        return true;
      } else if (value.compare("false") == 0 || value.compare("0") == 0 || value.compare("off") == 0 || value.compare("no") == 0) {
        return false;
      }
    }
    
    if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t>) {
      char* endPtr;
      auto val = strtol(value.c_str(), &endPtr, 10);
      if (*endPtr == '\0') {
        return static_cast<T>(val);
      } 
    }
    
    if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>) {
      char* endPtr;
      auto val = strtoul(value.c_str(), &endPtr, 10);
      if (*endPtr == '\0') {
        return static_cast<T>(val);
      }
    }
    
    if constexpr (std::is_same_v<T, int64_t>) {
      char* endPtr;
      auto val = strtoll(value.c_str(), &endPtr, 10);
      if (*endPtr == '\0') {
        return static_cast<T>(val);
      }
    }
    
    if constexpr (std::is_same_v<T, uint64_t>) {
      char* endPtr;
      auto val = strtoull(value.c_str(), &endPtr, 10);
      if (*endPtr == '\0') {
        return static_cast<T>(val);
      }
    }
    
    if constexpr (std::is_same_v<T, float>) {
      char* endPtr;
      auto val = strtof(value.c_str(), &endPtr);
      if (*endPtr == '\0') {
        return static_cast<T>(val);
      }
    }
    
    if constexpr (std::is_same_v<T, double>) {
      char* endPtr;
      auto val = strtod(value.c_str(), &endPtr);
      if (*endPtr == '\0') {
        return static_cast<T>(val);
      }
    }
    
    if constexpr (std::is_same_v<T, std::string>) {
      return std::move(value);
    }

    return def;
  }, def);
}
