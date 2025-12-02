#pragma once

#include <string>
#include <type_traits>
#include <variant>

namespace WrappedConfig {
  using ValueVariant = std::variant<
    std::monostate, // as null
    bool,
    int8_t, uint8_t,
    int16_t, uint16_t,
    int32_t, uint32_t,
#if WRAPPED_CONFIG_USE_LONG_LONG
    int64_t, uint64_t,
#endif
    int, float,
#if WRAPPED_CONFIG_USE_DOUBLE
    double,
#endif
    LazyString
  >;

  class Value : public ValueVariant {
    public:
      // Inherit all constructors from std::variant
      using ValueVariant::ValueVariant;
      using ValueVariant::operator=;
      using ValueVariant::index;

      friend bool operator==(const Value& lhs, const Value& rhs) {
        return static_cast<const ValueVariant&>(lhs) == static_cast<const ValueVariant&>(rhs);
      }

      friend bool operator!=(const Value& lhs, const Value& rhs) {
        return static_cast<const ValueVariant&>(lhs) != static_cast<const ValueVariant&>(rhs);
      }

      friend bool operator<(const Value& lhs, const Value& rhs) {
        return static_cast<const ValueVariant&>(lhs) < static_cast<const ValueVariant&>(rhs);
      }

      friend bool operator<=(const Value& lhs, const Value& rhs) {
        return static_cast<const ValueVariant&>(lhs) <= static_cast<const ValueVariant&>(rhs);
      }

      friend bool operator>(const Value& lhs, const Value& rhs) {
        return static_cast<const ValueVariant&>(lhs) > static_cast<const ValueVariant&>(rhs);
      }

      friend bool operator>=(const Value& lhs, const Value& rhs) {
        return static_cast<const ValueVariant&>(lhs) >= static_cast<const ValueVariant&>(rhs);
      }

      template <typename T>
      constexpr auto is() const noexcept {
        return std::holds_alternative<T>(*this);
      }

      constexpr auto isNull() const noexcept {
        return is<std::monostate>();
      }

      static const Value& null() {
        static const Value value{std::monostate{}};
        return value;
      }

      template <typename T>
        requires std::same_as<T, Value>
      const Value& as() const {
        return *this;
      }

      template <typename T>
        requires std::same_as<T, std::string>
      std::string as() const {
        return std::visit([](auto&& value) -> std::string {
          using ValueType = std::decay_t<decltype(value)>;

          if constexpr (std::same_as<ValueType, bool>) {
            return value ? WRAPPED_CONFIG_VALUE_TRUE : WRAPPED_CONFIG_VALUE_FALSE;

          } else if constexpr (std::is_arithmetic_v<ValueType>) {
            return std::to_string(value);

          } else if constexpr (std::same_as<ValueType, LazyString>) {
            return std::string{value.c_str()};
          }

          return {};
        }, static_cast<const ValueVariant&>(*this));
      }

      template <typename T>
        requires std::same_as<T, const char*>
      const char* as() const {
        return std::visit([](auto&& value) -> const char* {
          using ValueType = std::decay_t<decltype(value)>;

          if constexpr (std::same_as<ValueType, bool>) {
            return value ? WRAPPED_CONFIG_VALUE_TRUE : WRAPPED_CONFIG_VALUE_FALSE;

          } else if constexpr (std::is_arithmetic_v<ValueType>) {
            // Dangerous! Use only if you understand what you are doing.
            thread_local auto tmp = std::to_string(value);
            return tmp.c_str();

          } else if constexpr (std::same_as<ValueType, LazyString>) {
            return value.c_str();
          }

          return nullptr;
        }, static_cast<const ValueVariant&>(*this));
      }

      template <typename T>
        requires (!std::is_arithmetic_v<T> && !std::same_as<T, std::monostate> && !std::same_as<T, std::string> && !std::same_as<T, const char*>)
      const T& as() const {
        if (std::holds_alternative<T>(*this)) {
          return std::get<T>(*this);
        }

        throw std::runtime_error("Invalid type conversion");
      }

      template <typename T>
        requires std::is_arithmetic_v<T>
      T as() const {
        if (std::holds_alternative<T>(*this)) {
          return std::get<T>(*this);
        }

        return std::visit([&](auto&& value) -> T {
          using ValueType = std::decay_t<decltype(value)>;

          if constexpr (std::same_as<ValueType, bool>) {
            return static_cast<T>(value ? 1 : 0);

          } else if constexpr (std::is_arithmetic_v<ValueType>) {
            if constexpr (!std::same_as<T, bool>) {
              if (value < std::numeric_limits<T>::lowest()) {
                return std::numeric_limits<T>::lowest();

              } else if (value > std::numeric_limits<T>::max()) {
                return std::numeric_limits<T>::max();
              }
            }

            return static_cast<T>(value);

          } else if constexpr (std::same_as<ValueType, LazyString>) {
            Value parsedValue = fromString<T>(value.c_str());

            if (!parsedValue.isNull()) {
              return parsedValue.as<T>();
            }
          }

          throw std::runtime_error("Invalid type conversion");
        }, static_cast<const ValueVariant&>(*this));
      }

      template <typename T>
      static Value fromString(const char* value) {
        if constexpr (std::is_same_v<T, LazyString>) {
          return LazyString{value};

        } else if (value != nullptr && value[0] != '\0') {
          if constexpr (std::is_same_v<T, bool>) {
#if WRAPPED_CONFIG_EXTENDED_BOOL_VALUE_PARSING
            if (strcmp(value, WRAPPED_CONFIG_VALUE_TRUE) == 0 || strcmp(value, "1") == 0 || strcmp(value, "on") == 0 || strcmp(value, "yes") == 0) {
              return true;

            } else if (strcmp(value, WRAPPED_CONFIG_VALUE_FALSE) == 0 || strcmp(value, "0") == 0 || strcmp(value, "off") == 0 || strcmp(value, "no") == 0) {
              return false;
            }
#else
            if (strcmp(value, WRAPPED_CONFIG_VALUE_TRUE) == 0) {
              return true;

            } else if (strcmp(value, WRAPPED_CONFIG_VALUE_FALSE) == 0) {
              return false;
            }
#endif

          } else if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> ||
                               std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
            char* endPtr;
            auto val = std::strtol(value, &endPtr, 10);
            if (*endPtr == '\0') {
              return static_cast<T>(val);
            }

          } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                               std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>) {
            char* endPtr;
            auto val = strtoul(value, &endPtr, 10);
            if (*endPtr == '\0') {
              return static_cast<T>(val);
            }

#if WRAPPED_CONFIG_USE_LONG_LONG
          } else if constexpr (std::is_same_v<T, int64_t>) {
            char* endPtr;
            auto val = strtoll(value, &endPtr, 10);
            if (*endPtr == '\0') {
              return static_cast<T>(val);
            }

          } else if constexpr (std::is_same_v<T, uint64_t>) {
            char* endPtr;
            auto val = strtoull(value, &endPtr, 10);
            if (*endPtr == '\0') {
              return static_cast<T>(val);
            }
#endif

          } else if constexpr (std::is_same_v<T, float>) {
            char* endPtr;
            auto val = strtof(value, &endPtr);
            if (*endPtr == '\0') {
              return static_cast<T>(val);
            }

#if WRAPPED_CONFIG_USE_DOUBLE
          } else if constexpr (std::is_same_v<T, double>) {
            char* endPtr;
            auto val = strtod(value, &endPtr);
            if (*endPtr == '\0') {
              return static_cast<T>(val);
            }
#endif
          }
        }

        return null();
      }

      static Value fromString(const char* value, const Value& defaultValue) {
        auto variant = std::visit([&](auto&& def) -> Value {
          using T = std::decay_t<decltype(def)>;
          return Value::fromString<T>(value);
        }, defaultValue);

        return !variant.isNull() ? variant : defaultValue;
      }
  };
}
