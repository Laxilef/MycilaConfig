
#pragma once

#include "Value.h"

namespace WrappedConfig {
  class Item {
    public:
      Item(const char* key, Value&& defaultValue) : _key(key), _defaultValue(std::move(defaultValue)) {}
      Item(Item&& other) noexcept : _key(other._key), _defaultValue(std::move(other._defaultValue)), _value(std::move(other._value)) {}
      Item(const Item&) = delete;
      Item& operator=(const Item&) = delete;
      Item& operator=(Item&& other) noexcept {
        _key = other._key;
        _defaultValue = std::move(other._defaultValue);
        _value = std::move(other._value);

        return *this;
      }

      bool isPassword() const {
        size_t length = strlen(_key);
        if (length < sizeof(WRAPPED_CONFIG_KEY_PASSWORD_SUFFIX) - 1) {
          return false;
        }

        return strcmp(_key + length - sizeof(WRAPPED_CONFIG_KEY_PASSWORD_SUFFIX) - 1, WRAPPED_CONFIG_KEY_PASSWORD_SUFFIX) == 0;
      }

      bool isEnabled() const {
        size_t length = strlen(_key);
        if (length < sizeof(WRAPPED_CONFIG_KEY_ENABLED_SUFFIX) - 1) {
          return false;
        }

        return strcmp(_key + length - sizeof(WRAPPED_CONFIG_KEY_ENABLED_SUFFIX) - 1, WRAPPED_CONFIG_KEY_ENABLED_SUFFIX) == 0;
      }

      const char* getKey() const {
        return _key;
      }

      const Value& getDefaultValue() const {
        return _defaultValue;
      }

      bool hasValue() const {
        return !_value.isNull();
      }

      const Value& getValue() const {
        return _value;
      }

      Item& setValue(const Value& value) {
        _value = std::move(value);
        return *this;
      }

      void clearValue() {
        _value = Value::null();
      }

    private:
      const char* _key;
      Value _defaultValue;
      Value _value;
  };
}
