// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include "MycilaConfig.h"

#include <assert.h>

const Mycila::ValueVariant Mycila::WrappedConfig::empty = std::monostate{};

void Mycila::WrappedConfig::begin(const char* name) {
  assert(!_began);

  ESP_LOGI(TAG, "Initializing Config System: %s...", name);

  // sort keys
  std::sort(
    _keys.begin(), _keys.end(),
    [](const char* a, const char* b) {
      return strcmp(a, b) < 0;
    }
  );
  
  // begin storage
  assert(_storage->begin(name));

  // load from storage
  for(auto& [key, value] : _storage->load(_keys)) {
    _cache[key] = std::move(toVariant(value, _defaults[key]));
    ESP_LOGD(TAG, "begin(): loaded '%s' = '%s'", key, toString(_cache[key]).c_str());
  }

  _began = true;
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
  auto pKey = keyRef(key);
  if (pKey == nullptr) {
    ESP_LOGW(TAG, "setValidator(%s): Unknown key!", key);
    return false;
  }

  if (callback) {
    _validators[pKey] = callback;
    ESP_LOGD(TAG, "setValidator(%s, callback)", pKey);
  } else {
    _validators.erase(pKey);
    ESP_LOGD(TAG, "setValidator(%s, nullptr)", pKey);
  }

  return true;
}

bool Mycila::WrappedConfig::exists(const char* key) const {
  assert(_began);

  auto it = std::lower_bound(
    _keys.begin(), _keys.end(), key,
    [](const char* a, const char* b) {
      return strcmp(a, b) < 0;
    }
  );

  return it != _keys.end() && strcmp(*it, key) == 0;
}

void Mycila::WrappedConfig::configure(const char* key, ValueVariant defaultValue) {
  assert(!_began);

  _keys.push_back(key);
  _defaults[key] = std::move(defaultValue);
  ESP_LOGD(TAG, "Config Key '%s' defaults to '%s'", key, toString(_defaults[key]).c_str());
}

const Mycila::ValueVariant& Mycila::WrappedConfig::get(const char* key) const {
  assert(_began);

  // is it a real key ?
  auto pKey = keyRef(key);
  if (pKey == nullptr) {
    ESP_LOGW(TAG, "get(%s): Unknown key!", key);
    return empty;
  }

  // check if we have a cached value
  auto it = _cache.find(pKey);
  if (it != _cache.end()) {
    return it->second;
  }

  // key in storage exists ?
  if (_storage->exists(pKey)) {
    _cache[pKey] = _storage->get(pKey, _defaults.at(pKey));
    ESP_LOGD(TAG, "get(%s): Key cached", pKey);
    return _cache[pKey];
  }

  // key does not exist, or not assigned to a value
  _cache[pKey] = _defaults.at(pKey);
  return _cache[pKey];
}

const char* Mycila::WrappedConfig::get(const char* key, const char* defaultValue) const {
  const auto& value = get(key);
  if (value.index() == 0 || !std::holds_alternative<std::string>(value)) {
    return defaultValue;
  }

  return std::get<std::string>(value).c_str();
}

bool Mycila::WrappedConfig::isEqual(const char* key, const ValueVariant& value) const {
  assert(_began);

  return get(key) == value;
}

const Mycila::WrappedConfig::SetResult Mycila::WrappedConfig::set(const char* key, ValueVariant value, bool fireChangeCallback) {
  assert(_began);

  // check if the key is valid
  auto pKey = keyRef(key);
  if (pKey == nullptr) {
    ESP_LOGW(TAG, "set(%s): UNKNOWN_KEY", key);
    return Result::UNKNOWN_KEY;
  }

  // check if the type valid
  if (value.index() != _defaults.at(pKey).index()) {
    ESP_LOGD(TAG, "set(%s): TYPE_MISMATCH", pKey);
    return Result::TYPE_MISMATCH;
  }

  // check if the value is the same as the default
  if (value == _defaults.at(pKey)) {
    ESP_LOGD(TAG, "set(%s): SAME_AS_DEFAULT", pKey);
    return Result::SAME_AS_DEFAULT;
  }

  // check if the value is the same as the current
  if (value == get(pKey)) {
    ESP_LOGD(TAG, "set(%s): ALREADY_PERSISTED", pKey);
    return Result::ALREADY_PERSISTED;
  }

  // check if we have a global validator
  // and check if the value is valid
  if (_globalValidatorCallback && !_globalValidatorCallback(pKey, value)) {
    ESP_LOGD(TAG, "set(%s): INVALID_VALUE", pKey);
    return Result::INVALID_VALUE;
  }

  // check if we have a specific validator for the key
  // and check if the value is valid
  auto it = _validators.find(pKey);
  if (it != _validators.end() && !it->second(pKey, value)) {
    ESP_LOGD(TAG, "set(%s): INVALID_VALUE", pKey);
    return Result::INVALID_VALUE;
  }

  // update failed ?
  if (!_storage->set(pKey, value)) {
    ESP_LOGD(TAG, "set(%s): FAIL_ON_WRITE", pKey);
    return Result::FAIL_ON_WRITE;
  }

  _cache[pKey] = std::move(value);
  ESP_LOGD(TAG, "set(%s): PERSISTED", pKey);
  if (fireChangeCallback && _changeCallback)
    _changeCallback(pKey, _cache[pKey]);

  return Result::PERSISTED;
}

bool Mycila::WrappedConfig::set(const std::map<const char*, ValueVariant>& settings, bool fireChangeCallback) {
  assert(_began);

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
  assert(_began);

  // check if the key is valid
  auto pKey = keyRef(key);
  if (pKey == nullptr) {
    ESP_LOGW(TAG, "unset(%s): Unknown key!", key);
    return false;
  }

  // key not removed
  if (!_storage->unset(pKey)) {
    ESP_LOGW(TAG, "unset(%s): Failed to unset!", pKey);
    return false;
  }

  // key there and to remove
  _cache.erase(pKey);
  ESP_LOGD(TAG, "unset(%s)", pKey);
  if (fireChangeCallback && _changeCallback)
    _changeCallback(pKey, empty);

  return true;
}

void Mycila::WrappedConfig::backup(Print& out) {
  assert(_began);

  for (auto& key : _keys) {
    auto value = get(key);
    out.print(key);
    out.print('=');
    out.print(toString(value).c_str());
    out.print("\n");
  }
}

bool Mycila::WrappedConfig::restore(const char* data) {
  assert(_began);

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

      auto pKey = keyRef(key);
      if (pKey == nullptr) {
        std::string val(start, end - start);
        settings[pKey] = toVariant(val, _defaults.at(pKey));
      }
    }
  }
  return restore(settings);
}

bool Mycila::WrappedConfig::restore(const std::map<const char*, ValueVariant>& settings) {
  assert(_began);

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
  assert(_began);

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
  assert(_began);

  auto it = std::lower_bound(
    _keys.begin(), _keys.end(), buffer,
    [](const char* a, const char* b) {
      return strcmp(a, b) < 0;
    }
  );

  return it != _keys.end() && strcmp(*it, buffer) == 0
    ? static_cast<const char*>(*it)
    : nullptr;
}

#ifdef MYCILA_JSON_SUPPORT
bool Mycila::WrappedConfig::toJson(JsonObject root, const char* key) {
  assert(_began);

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
  }, get(key));
}

void Mycila::WrappedConfig::toJson(JsonObject root) {
  assert(_began);

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
