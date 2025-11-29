// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include "WrappedConfig.h"

#include <assert.h>

WrappedConfig::WrappedConfig& WrappedConfig::WrappedConfig::begin(const char* name) {
  assert(!_began);

  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "Initializing Config System: %s...", name);

  // sort items by key
  std::sort(
    _items.begin(), _items.end(),
    [](const Item& a, const Item& b) {
      return strcmp(a.getKey(), b.getKey()) < 0;
    }
  );

  _items.shrink_to_fit();
  _validators.shrink_to_fit();

  // begin storage
  assert(_storage->begin(name));
  _storage->setWrapper(this);
  _began = true;

  // load from storage
  for(auto& [key, value] : _storage->load()) {
    ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "FS KEY %s=%s", key, value.c_str());

    auto* item = getItem(key);
    if (item == nullptr) {
      ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "FS NO KEY FOR %s", key);
      continue;
    }

    auto variant = std::visit([&](auto&& def) -> Value {
      using T = std::decay_t<decltype(def)>;
      return Value::fromString<T>(value.c_str());
    }, item->getDefaultValue());

    if (!variant.isNull()) {
      item->setValue(variant);
      ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "begin(): loaded '%s' = '%s'", item->getKey(), item->getValue().toString().data());
    } else {
      ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "FS NO VARIANT FOR %s", key);
    }
  }

  return *this;
}

bool WrappedConfig::WrappedConfig::setValidator(ConfigValidatorCallback callback) {
  if (callback) {
    _globalValidatorCallback = std::move(callback);
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(callback)");
  } else {
    _globalValidatorCallback = nullptr;
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(nullptr)");
  }
  return true;
}

bool WrappedConfig::WrappedConfig::setValidator(const char* key, ConfigValidatorCallback callback) {
  // find item
  auto* item = getItem(key);
  if (item == nullptr) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "setValidator(%s): Unknown key!", key);
    return false;
  }

  auto it = std::find_if(
    _validators.begin(), _validators.end(),
    [&](const auto& p) {
      return p.first == item->getKey();
    }
  );

  if (callback) {
    if (it != _validators.end()) {
      it->second = std::move(callback);
    } else {
      _validators.emplace_back(item->getKey(), std::move(callback));
    }

    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(%s, callback)", item->getKey());

  } else {
    if (it != _validators.end()) {
      _validators.erase(it);
    }

    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(%s, nullptr)", item->getKey());
  }

  return true;
}

bool WrappedConfig::WrappedConfig::exists(const char* key) const {
  assert(_began);

  return getItem(key) != nullptr;
}

bool WrappedConfig::WrappedConfig::isEqual(const char* key, const Value& variant) {
  assert(_began);

  return get(key) == variant;
}

const WrappedConfig::Result WrappedConfig::WrappedConfig::set(const char* key, Value variant, bool fireChangeCallback) {
  assert(_began);

  // find item
  auto* item = getItem(key);
  if (item == nullptr) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "set(%s): UNKNOWN_KEY", key);
    return Status::UNKNOWN_KEY;
  }

  // check if the type valid
  if (variant.index() != item->getDefaultValue().index()) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): INVALID_TYPE", item->getKey());
    return Status::INVALID_TYPE;
  }

  // check if the value is the same as the default
  if (variant == item->getDefaultValue()) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): SAME_AS_DEFAULT", item->getKey());

    if (_storage->exists(item->getKey())) {
      _storage->unset(item->getKey());
    }

    item->clearValue();

    return Status::SAME_AS_DEFAULT;
  }

  // check if the value is the same as the current
  if (item->hasValue() && variant == item->getValue()) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): SAME_AS_PERSISTED", item->getKey());
    return Status::SAME_AS_PERSISTED;
  }

  // check if we have a global validator
  // and check if the value is valid
  if (_globalValidatorCallback && !_globalValidatorCallback(item->getKey(), variant)) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): INVALID_VALUE", item->getKey());
    return Status::INVALID_VALUE;
  }

  // check if we have a specific validator for the key
  // and check if the value is valid
  auto vit = std::find_if(
    _validators.begin(),
    _validators.end(),
    [&](const auto& p) {
      return p.first == item->getKey();
    }
  );
  if (vit != _validators.end() && !vit->second(item->getKey(), variant)) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): INVALID_VALUE", item->getKey());
    return Status::INVALID_VALUE;
  }

  // update failed ?
  if (!_storage->set(item->getKey(), variant)) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): FAIL_ON_WRITE", item->getKey());
    return Status::FAIL_ON_WRITE;
  }

  item->setValue(variant);
  ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): PERSISTED", item->getKey());

  if (fireChangeCallback && _changeCallback) {
    _changeCallback(item->getKey(), item->getValue());
  }

  return Status::PERSISTED;
}

bool WrappedConfig::WrappedConfig::set(const std::map<const char*, Value>& settings, bool fireChangeCallback) {
  assert(_began);

  bool updates = false;

  // start restoring settings
  for (auto& item : _items) {
    auto key = item.getKey();
    if (!item.isEnabled() && settings.find(key) != settings.end()) {
      updates |= set(key, settings.at(key), fireChangeCallback) == Status::PERSISTED;
    }
  }

  // then restore settings enabling/disabling a feature
  for (auto& item : _items) {
    auto key = item.getKey();
    if (item.isEnabled() && settings.find(key) != settings.end()) {
      updates |= set(key, settings.at(key), fireChangeCallback) == Status::PERSISTED;
    }
  }

  return updates;
}

const WrappedConfig::Result WrappedConfig::WrappedConfig::unset(const char* key, bool fireChangeCallback) {
  assert(_began);

  // find item
  auto* item = getItem(key);
  if (item == nullptr) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "unset(%s): UNKNOWN_KEY", key);
    return Status::UNKNOWN_KEY;
  }

  // key not removed
  if (!_storage->unset(item->getKey())) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "unset(%s): FAIL_ON_REMOVE", item->getKey());
    return Status::FAIL_ON_REMOVE;
  }

  // remove from cache
  if (item->hasValue()) {
    item->clearValue();
  }

  ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "unset(%s) REMOVED", item->getKey());

  if (fireChangeCallback && _changeCallback) {
    _changeCallback(item->getKey(), item->getValue());
  }

  return Status::REMOVED;
}

void WrappedConfig::WrappedConfig::backup(Print& out) {
  assert(_began);

  for (auto& item : _items) {
    const auto& variant = get(item.getKey());
    out.print(item.getKey());
    out.print('=');
    out.print(variant.toString().data());
    out.print("\n");
  }
}

bool WrappedConfig::WrappedConfig::restore(const char* data) {
  assert(_began);

  std::map<const char*, Value> settings;
  for (auto& item : _items) {
    auto key = item.getKey();
    char* start = strstr(data, key);
    if (start) {
      start += strlen(key);
      if (*start != '=') {
        continue;
      }

      start++;
      char* end = strchr(start, '\r');
      if (!end) {
        end = strchr(start, '\n');
      }

      if (!end) {
        ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "restore(%s): Invalid data!", key);
        return false;
      }

      std::string val(start, end - start);
      auto variant = std::visit([&](auto&& def) -> Value {
        using T = std::decay_t<decltype(def)>;
        return Value::fromString<T>(val.c_str());
      }, item.getDefaultValue());

      if (!variant.isNull()) {
        settings[key] = variant;
      }
    }
  }
  return restore(settings);
}

bool WrappedConfig::WrappedConfig::restore(const std::map<const char*, Value>& settings) {
  assert(_began);

  ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "Restoring %d settings...", settings.size());
  bool restored = set(settings, false);
  if (restored) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "Config restored");
    if (_restoreCallback)
      _restoreCallback();
  } else
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "No change detected");
  return restored;
}

void WrappedConfig::WrappedConfig::clear() {
  assert(_began);

  // clear storage
  _storage->clear();

  // clear all values
  for(auto& item : _items) {
    item.clearValue();
  }
}

#ifdef MYCILA_JSON_SUPPORT
bool WrappedConfig::WrappedConfig::toJson(JsonObject root, const char* key) {
  assert(_began);

  return std::visit([&](auto&& value) -> bool {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same_v<T, bool> || std::is_arithmetic_v<T>) {
      root[key] = value;
      return true;

    } else if constexpr (std::is_same_v<T, LazyString>) {
#ifdef WRAPPED_CONFIG_PASSWORD_MASK
      root[key] = !isPasswordKey(key) ? value.c_str() : WRAPPED_CONFIG_PASSWORD_MASK;
#else
      root[key] = value.c_str();
#endif
      return true;
    }

    return false;
  }, get(key));
}

void WrappedConfig::WrappedConfig::toJson(JsonObject root) {
  assert(_began);

  for (auto& item : _items) {
    toJson(root, item.getKey());
  }
}
#endif

const WrappedConfig::Value& WrappedConfig::WrappedConfig::_findValue(const std::vector<ValuePair>& source, const char* key) const {
  auto it = std::find_if(
    source.begin(), source.end(),
    [key](const auto& p) {
      return p.first == key;
    }
  );

  return it != source.end() ? it->second : Value::null();
}

WrappedConfig::Value& WrappedConfig::WrappedConfig::_upsertValue(std::vector<ValuePair>& target, const char* key, const Value& value) {
  auto it = std::find_if(
    target.begin(), target.end(),
    [key](const auto& p) {
      return p.first == key;
    }
  );

  if (it != target.end()) {
    it->second = std::move(value);
    return it->second;

  } else {
    return target.emplace_back(key, std::move(value)).second;
  }
}

void WrappedConfig::WrappedConfig::_eraseValue(std::vector<ValuePair>& target, const char* key) {
  auto it = std::find_if(target.begin(), target.end(), [key](const auto& p) { return p.first == key; });
  if (it != target.end()) {
    target.erase(it);
  }
}

const WrappedConfig::ConfigValidatorCallback* WrappedConfig::WrappedConfig::_findValidator(const char* key) const {
  auto it = std::find_if(
    _validators.begin(), _validators.end(),
    [key](const auto& p) {
      return p.first == key;
    }
  );

  return it != _validators.end() ? &(it->second) : nullptr;
}

void WrappedConfig::WrappedConfig::_upsertValidator(const char* key, ConfigValidatorCallback callback) {
  auto it = std::find_if(
    _validators.begin(), _validators.end(),
    [key](const auto& p) {
      return p.first == key;
    }
  );

  if (it != _validators.end()) {
    it->second = std::move(callback);

  } else if (callback) {
    _validators.emplace_back(key, std::move(callback));
  }
}

void WrappedConfig::WrappedConfig::_eraseValidator(const char* key) {
  auto it = std::find_if(
    _validators.begin(), _validators.end(),
    [key](const auto& p) {
      return p.first == key;
    }
  );

  if (it != _validators.end()) {
    _validators.erase(it);
  }
}
