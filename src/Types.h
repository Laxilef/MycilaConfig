// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <functional>
#include <variant>
#include <string>

#include "LazyString.h"
#include "Value.h"

namespace WrappedConfig {
  typedef std::function<void(const char* key, const Value& newValue)> ConfigChangeCallback;
  typedef std::function<void()> ConfigRestoredCallback;
  typedef std::function<bool(const char* key, const Value& newValue)> ConfigValidatorCallback;

  using ValidatorPair = std::pair<const char*, ConfigValidatorCallback>;

  enum class Status {
    PERSISTED,
    REMOVED,
    UNKNOWN_KEY,
    SAME_AS_PERSISTED,
    SAME_AS_DEFAULT,
    INVALID_VALUE,
    INVALID_TYPE,
    FAIL_ON_WRITE,
    FAIL_ON_REMOVE
  };

  class Result {
    public:
      constexpr Result(Status status) noexcept : _status(status) {} // NOLINT

      constexpr operator bool() const {
        return _status == Status::PERSISTED || _status == Status::REMOVED ||
               _status == Status::SAME_AS_PERSISTED || _status == Status::SAME_AS_DEFAULT;
      }

      constexpr operator Status() const {
        return _status;
      }

      constexpr bool operator==(const Status& other) const {
        return _status == other;
      }

      constexpr bool operator!=(const Status& other) const {
        return _status != other;
      }

    private:
      Status _status;
  };

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