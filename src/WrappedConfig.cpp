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
  auto preloaded = _storage->preload();
  ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "begin(): preloaded %zu items", preloaded);

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

WrappedConfig::Item* WrappedConfig::WrappedConfig::getItem(const char* key) const {
  auto it = std::lower_bound(
    _items.begin(), _items.end(), key,
    [](const Item& item, const char* str) {
      return strcmp(item.getKey(), str) < 0;
    }
  );

  return (it != _items.end() && (it->getKey() == key || strcmp(it->getKey(), key) == 0))
    ? &(*it)
    : nullptr;
}

const WrappedConfig::Value& WrappedConfig::WrappedConfig::get(const char* key) {
  assert(_began);

  // find item
  auto* pItem = getItem(key);
  if (pItem == nullptr) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "get(%s): Unknown key!", key);
    return Value::null();
  }

  // check if we have a cached value
  if (pItem->hasValue()) {
    return pItem->getValue();
  }

  // key in storage exists ?
  if (_storage->exists(pItem->getKey())) {
    auto variant = _storage->get(pItem->getKey(), pItem->getDefaultValue());

    if (!variant.isNull()) {
      pItem->setValue(variant);
      ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "get(%s): Key cached", pItem->getKey());
      return pItem->getValue();
    }
  }

  // key does not exist, or not assigned to a value
  return pItem->getDefaultValue();
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

  char buffer[256];
  const char* pData = data;
  while (*pData != '\0') {
    const char* pLineEnd = strchr(pData, '\n');
    if (pLineEnd == nullptr) {
      pLineEnd = pData + strlen(pData);
    }

    size_t lineLength = pLineEnd - pData;
    if (lineLength == 0) {
      pData = pLineEnd + 1;
      continue;
    }

    if (lineLength >= sizeof(buffer) - 1) {
      ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "restore(...): Line too long, skipping");
      pData = pLineEnd + 1;
      continue;
    }

    strncpy(buffer, pData, lineLength);
    buffer[lineLength] = '\0';

    auto parsedKey = parseKey(buffer);
    if (parsedKey.empty()) {
      pData = pLineEnd + 1;
      continue;
    }

    auto parsedKeyStr = std::string{parsedKey};
    auto* pItem = getItem(parsedKeyStr.c_str());
    parsedKeyStr.clear();

    if (pItem == nullptr) {
      pData = pLineEnd + 1;
      continue;
    }

    auto parsedValueStr = std::string{parseValue(buffer)};
    auto variant = std::visit([&](auto&& def) -> Value {
      using T = std::decay_t<decltype(def)>;
      return Value::fromString<T>(parsedValueStr.c_str());
    }, pItem->getDefaultValue());
    parsedValueStr.clear();

    if (variant.isNull()) {
      pData = pLineEnd + 1;
      continue;
    }

    settings[pItem->getKey()] = variant;

    pData = pLineEnd;
    while (*pData == '\n' || *pData == '\r') {
      pData++;
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
    if (_restoreCallback) {
      _restoreCallback();
    }

  } else {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "No change detected");
  }

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

#ifdef WRAPPED_CONFIG_JSON_SUPPORT
bool WrappedConfig::WrappedConfig::toJson(JsonObject root, const char* key) {
  assert(_began);

  // find item
  auto* pItem = getItem(key);
  if (pItem == nullptr) {
    return false;
  }

  return std::visit([&](auto&& value) -> bool {
    using T = std::decay_t<decltype(value)>;

    if constexpr (std::is_same_v<T, bool> || std::is_arithmetic_v<T>) {
      root[key] = value;
      return true;

    } else if constexpr (std::is_same_v<T, LazyString>) {
#ifdef WRAPPED_CONFIG_PASSWORD_MASK
      root[key] = pItem->isPassword() ? value.c_str() : WRAPPED_CONFIG_PASSWORD_MASK;
#else
      root[key] = value.c_str();
#endif
      return true;
    }

    return false;
  }, pItem->hasValue() ? pItem->getValue() : get(key));
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
