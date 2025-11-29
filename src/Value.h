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

      static const Value& null() {
        static const Value value{std::monostate{}};
        return value;
      }

      bool isNull() const noexcept {
        return std::holds_alternative<std::monostate>(*this);
      }

      // get the value held as std::string (only for LazyString type)
      template <typename T, std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
      std::string as() const {
        if (std::holds_alternative<LazyString>(*this)) {
          return std::string{std::get<LazyString>(*this).c_str()};
        }
        throw std::runtime_error("Invalid type conversion");
      }

      // get the value held as const char* (only for LazyString type)
      template <typename T, std::enable_if_t<std::is_same_v<T, const char*>, int> = 0>
      const char* as() const {
        if (std::holds_alternative<LazyString>(*this)) {
          return std::get<LazyString>(*this).c_str();
        }
        throw std::runtime_error("Invalid type conversion");
      }

      // get the value held as type T (for variant alternatives or Value itself)
      template <typename T = Value, std::enable_if_t<!std::is_same_v<T, std::string> && !std::is_same_v<T, const char*>, int> = 0>
      const T& as() const {
        if constexpr (std::is_same_v<T, Value>) {
          return *this;

        } else if (std::holds_alternative<T>(*this)) {
          return std::get<T>(*this);

        }/* else if constexpr (std::is_arithmetic_v<T>) {
          return std::visit([&](auto&& value) -> T {
            using VT = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<VT, bool>) {
              return static_cast<T>(value ? 1 : 0);

            } else if constexpr (std::is_arithmetic_v<VT>) {
              if (value < std::numeric_limits<T>::lowest()) {
                return std::numeric_limits<T>::lowest();

              } else if (value > std::numeric_limits<T>::max()) {
                return std::numeric_limits<T>::max();

              } else {
                return static_cast<T>(value);
              }

            } else {
              throw std::runtime_error("Invalid type conversion");
            }

          }, static_cast<const ValueVariant&>(*this));
        }*/

        throw std::runtime_error("Invalid type conversion");
      }

      // convert to a string any value held
      std::string toString() const {
        return std::visit([](auto&& value) -> std::string {
          using T = std::decay_t<decltype(value)>;

          if constexpr (std::is_same_v<T, bool>) {
            return value ? WRAPPED_CONFIG_VALUE_TRUE : WRAPPED_CONFIG_VALUE_FALSE;

          } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(value);

          } else if constexpr (std::is_same_v<T, LazyString>) {
            return std::string(value.c_str());

          } else {
            return std::string{};
          }
        }, static_cast<const ValueVariant&>(*this));
      }

      /*
      std::string_view toString() const {
        return std::visit([](auto&& value) -> std::string_view {
#if WRAPPED_CONFIG_USE_LONG_LONG || WRAPPED_CONFIG_USE_DOUBLE
          thread_local char buf[32];
#else
          thread_local char buf[16];
#endif

          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, bool>) {
            return value ? WRAPPED_CONFIG_VALUE_TRUE : WRAPPED_CONFIG_VALUE_FALSE;

          } else if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> ||
                               std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
            snprintf(buf, sizeof(buf), "%" PRId32, static_cast<int32_t>(value));
            return buf;

          } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                               std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>) {
            snprintf(buf, sizeof(buf), "%" PRIu32, static_cast<uint32_t>(value));
            return buf;

#if WRAPPED_CONFIG_USE_LONG_LONG
          } else if constexpr (std::is_same_v<T, int64_t>) {
            snprintf(buf, sizeof(buf), "%" PRId64, static_cast<int64_t>(value));
            return buf;

          } else if constexpr (std::is_same_v<T, uint64_t>) {
            snprintf(buf, sizeof(buf), "%" PRIu64, static_cast<uint64_t>(value));
            return buf;
#endif

          } else if constexpr (std::is_same_v<T, float>) {
            snprintf(buf, sizeof(buf), "%f", value);
            return buf;

#if WRAPPED_CONFIG_USE_DOUBLE
          } else if constexpr (std::is_same_v<T, double>) {
            snprintf(buf, sizeof(buf), "%g", value);
            return buf;
#endif

          } else if constexpr (std::is_same_v<T, LazyString>) {
            return value.c_str();
            return buf;

          } else {
            buf[0] = '\0';
            return buf;
          }
        }, static_cast<const ValueVariant&>(*this));
      }*/

      template <typename T>
      static Value fromString(const char* value) {
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

        } else if constexpr (std::is_same_v<T, LazyString>) {
          return LazyString{value};
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
} // namespace WrappedConfig
