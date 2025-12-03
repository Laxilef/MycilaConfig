#include "Config.h"
#include "Utils.h"

#include <assert.h>


WrappedConfig::Config::Config(Storage::Base& storage, uint16_t reserve) : _storage(storage) {
  _items.reserve(reserve);
}

WrappedConfig::Config::~Config() {
  flush();
}

WrappedConfig::Config& WrappedConfig::Config::begin(const char* name) {
  assert(!_began);

  ESP_LOGI(WRAPPED_CONFIG_LOG_TAG, "Initializing Config System: %s...", name);

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
  assert(_storage.begin(name));
  _storage.setWrapper(this);
  _began = true;

  // load from storage
  auto preloaded = _storage.preload();
  ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "begin(): preloaded %zu items", preloaded);

  return *this;
}

bool WrappedConfig::Config::setValidator(ConfigValidatorCallback callback) {
  if (callback) {
    _globalValidatorCallback = std::move(callback);
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "setValidator(callback)");
  } else {
    _globalValidatorCallback = nullptr;
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "setValidator(nullptr)");
  }
  return true;
}

bool WrappedConfig::Config::setValidator(const char* key, ConfigValidatorCallback callback) {
  // find item
  auto* pItem = getItem(key);
  if (pItem == nullptr) {
    ESP_LOGW(WRAPPED_CONFIG_LOG_TAG, "setValidator(%s): Unknown key!", key);
    return false;
  }

  auto it = std::find_if(
    _validators.begin(), _validators.end(),
    [&](const auto& p) {
      return p.first == pItem->getKey();
    }
  );

  if (callback) {
    if (it != _validators.end()) {
      it->second = std::move(callback);
    } else {
      _validators.emplace_back(pItem->getKey(), std::move(callback));
    }

    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "setValidator(%s, callback)", pItem->getKey());

  } else {
    if (it != _validators.end()) {
      _validators.erase(it);
    }

    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "setValidator(%s, nullptr)", pItem->getKey());
  }

  return true;
}

bool WrappedConfig::Config::exists(const char* key) const {
  assert(_began);

  return getItem(key) != nullptr;
}

WrappedConfig::Item* WrappedConfig::Config::getItem(const char* key) const {
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

const WrappedConfig::Value& WrappedConfig::Config::get(const char* key) {
  assert(_began);

  // find item
  auto* pItem = getItem(key);
  if (pItem == nullptr) {
    ESP_LOGW(WRAPPED_CONFIG_LOG_TAG, "get(%s): Unknown key!", key);
    return Value::null();
  }

  // check if we have a cached value
  if (pItem->hasValue()) {
    return pItem->getValue();
  }

  // key in storage exists ?
  if (_storage.exists(pItem->getKey())) {
    auto variant = _storage.get(pItem->getKey(), pItem->getDefaultValue());

    if (!variant.isNull()) {
      pItem->setValue(variant);
      ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "get(%s): Key cached", pItem->getKey());
      return pItem->getValue();
    }
  }

  // key does not exist, or not assigned to a value
  return pItem->getDefaultValue();
}

bool WrappedConfig::Config::isEqual(const char* key, const Value& variant) {
  assert(_began);

  return get(key) == variant;
}

const WrappedConfig::Result WrappedConfig::Config::set(const char* key, Value variant, bool fireChangeCallback) {
  assert(_began);

  // find item
  auto* pItem = getItem(key);
  if (pItem == nullptr) {
    ESP_LOGW(WRAPPED_CONFIG_LOG_TAG, "set(%s): UNKNOWN_KEY", key);
    return Status::UNKNOWN_KEY;
  }

  // check if the type valid
  if (variant.index() != pItem->getDefaultValue().index()) {
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "set(%s): INVALID_TYPE", pItem->getKey());
    return Status::INVALID_TYPE;
  }

  // check if the value is the same as the default
  if (variant == pItem->getDefaultValue()) {
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "set(%s): SAME_AS_DEFAULT", pItem->getKey());

    if (_storage.exists(pItem->getKey())) {
      _storage.unset(pItem->getKey());
    }

    pItem->clearValue();

    return Status::SAME_AS_DEFAULT;
  }

  // check if the value is the same as the current
  if (pItem->hasValue() && variant == pItem->getValue()) {
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "set(%s): SAME_AS_PERSISTED", pItem->getKey());
    return Status::SAME_AS_PERSISTED;
  }

  // check if we have a global validator
  // and check if the value is valid
  if (_globalValidatorCallback && !_globalValidatorCallback(pItem->getKey(), variant)) {
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "set(%s): INVALID_VALUE", pItem->getKey());
    return Status::INVALID_VALUE;
  }

  // check if we have a specific validator for the key
  // and check if the value is valid
  auto vit = std::find_if(
    _validators.begin(),
    _validators.end(),
    [&](const auto& p) {
      return p.first == pItem->getKey();
    }
  );
  if (vit != _validators.end() && !vit->second(pItem->getKey(), variant)) {
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "set(%s): INVALID_VALUE", pItem->getKey());
    return Status::INVALID_VALUE;
  }

  // update failed ?
  if (!_storage.set(pItem->getKey(), variant)) {
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "set(%s): FAIL_ON_WRITE", pItem->getKey());
    return Status::FAIL_ON_WRITE;
  }

  pItem->setValue(variant);
  ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "set(%s): PERSISTED", pItem->getKey());

  if (fireChangeCallback && _changeCallback) {
    _changeCallback(pItem->getKey(), pItem->getValue());
  }

  return Status::PERSISTED;
}

bool WrappedConfig::Config::set(const std::map<const char*, Value>& settings, bool fireChangeCallback) {
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

const WrappedConfig::Result WrappedConfig::Config::unset(const char* key, bool fireChangeCallback) {
  assert(_began);

  // find item
  auto* pItem = getItem(key);
  if (pItem == nullptr) {
    ESP_LOGW(WRAPPED_CONFIG_LOG_TAG, "unset(%s): UNKNOWN_KEY", key);
    return Status::UNKNOWN_KEY;
  }

  // key not removed
  if (!_storage.unset(pItem->getKey())) {
    ESP_LOGW(WRAPPED_CONFIG_LOG_TAG, "unset(%s): FAIL_ON_REMOVE", pItem->getKey());
    return Status::FAIL_ON_REMOVE;
  }

  // remove from cache
  if (pItem->hasValue()) {
    pItem->clearValue();
  }

  ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "unset(%s) REMOVED", pItem->getKey());

  if (fireChangeCallback && _changeCallback) {
    _changeCallback(pItem->getKey(), pItem->getValue());
  }

  return Status::REMOVED;
}

void WrappedConfig::Config::backup(Print& out) {
  assert(_began);

  for (auto& item : _items) {
    const auto& variant = get(item.getKey());
    out.print(item.getKey());
    out.print('=');
    out.print(variant.as<const char*>());
    out.print("\n");
  }
}

bool WrappedConfig::Config::restore(const char* data) {
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
      ESP_LOGW(WRAPPED_CONFIG_LOG_TAG, "restore(...): Line too long, skipping");
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

bool WrappedConfig::Config::restore(const std::map<const char*, Value>& settings) {
  assert(_began);

  ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "Restoring %d settings...", settings.size());
  bool restored = set(settings, false);

  if (restored) {
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "Config restored");
    if (_restoreCallback) {
      _restoreCallback();
    }

  } else {
    ESP_LOGD(WRAPPED_CONFIG_LOG_TAG, "No change detected");
  }

  return restored;
}

void WrappedConfig::Config::clear() {
  assert(_began);

  // clear storage
  _storage.clear();

  // clear all values
  for(auto& item : _items) {
    item.clearValue();
  }
}

#ifdef WRAPPED_CONFIG_JSON_SUPPORT
bool WrappedConfig::Config::toJson(JsonObject root, const char* key) {
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

void WrappedConfig::Config::toJson(JsonObject root) {
  assert(_began);

  for (auto& item : _items) {
    toJson(root, item.getKey());
  }
}
#endif
