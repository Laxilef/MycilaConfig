
#pragma once

#include <functional>
#include <utility>

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
}
