#pragma once

#include <Preferences.h>

#include "Base.h"

namespace WrappedConfig {
  namespace Storage {
    class Preferences : public Base {
      public:
        bool begin(const char* name) override {
          return _prefs.begin(name, false);
        }

        bool end() override {
          _prefs.end();
          return true;
        }

        bool clear() override {
          return _prefs.clear();
        }

        virtual bool exists(const char* key) const override {
          return _prefs.isKey(key);
        }

        virtual bool unset(const char* key) override {
          return _prefs.isKey(key) && _prefs.remove(key);
        }

        virtual bool set(const char* key, const Value& value) override {
          // Limitation of the Preferences library:
          // https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32/api-reference/storage/nvs_flash.html#keys-and-values
          assert(strlen(key) <= 15);

          return std::visit([&](auto&& v) -> bool {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, bool>) {
              return _prefs.putBool(key, v);

            } else if constexpr (std::is_same_v<T, int8_t>) {
              return _prefs.putChar(key, static_cast<char>(v));

            } else if constexpr (std::is_same_v<T, uint8_t>) {
              return _prefs.putUChar(key, v);

            } else if constexpr (std::is_same_v<T, int16_t>) {
              return _prefs.putShort(key, static_cast<short>(v));

            } else if constexpr (std::is_same_v<T, uint16_t>) {
              return _prefs.putUShort(key, v);

            } else if constexpr (std::is_same_v<T, int32_t>) {
              return _prefs.putInt(key, v);

            } else if constexpr (std::is_same_v<T, uint32_t>) {
              return _prefs.putUInt(key, v);
#if WRAPPED_CONFIG_USE_LONG_LONG
            } else if constexpr (std::is_same_v<T, int64_t>) {
              return _prefs.putLong64(key, v);

            } else if constexpr (std::is_same_v<T, uint64_t>) {
              return _prefs.putULong64(key, v);
#endif
            } else if constexpr (std::is_same_v<T, int>) {
              return _prefs.putInt(key, v);

            } else if constexpr (std::is_same_v<T, float>) {
              return _prefs.putFloat(key, v);
#if WRAPPED_CONFIG_USE_DOUBLE
            } else if constexpr (std::is_same_v<T, double>) {
              return _prefs.putDouble(key, v);
#endif
            } else if constexpr (std::is_same_v<T, LazyString>) {
              return _prefs.putString(key, v.c_str()) == v.size();

            } else {
              return false;
            }
          }, value);
        }

        virtual Value get(const char* key, const Value& defaultValue) const override {
          if (!_prefs.isKey(key)) {
            return Value::null();
          }

          return std::visit([&](auto&& def) -> Value {
            using T = std::decay_t<decltype(def)>;

            if constexpr (std::is_same_v<T, bool>) {
              return _prefs.getBool(key, def);

            } else if constexpr (std::is_same_v<T, int8_t>) {
              return static_cast<int8_t>(_prefs.getChar(key, static_cast<char>(def)));

            } else if constexpr (std::is_same_v<T, uint8_t>) {
              return _prefs.getUChar(key, static_cast<unsigned char>(def));

            } else if constexpr (std::is_same_v<T, int16_t>) {
              return static_cast<int16_t>(_prefs.getShort(key, static_cast<short>(def)));

            } else if constexpr (std::is_same_v<T, uint16_t>) {
              return _prefs.getUShort(key, static_cast<unsigned short>(def));

            } else if constexpr (std::is_same_v<T, int32_t>) {
              return _prefs.getInt(key, def);

            } else if constexpr (std::is_same_v<T, uint32_t>) {
              return _prefs.getUInt(key, def);
#if WRAPPED_CONFIG_USE_LONG_LONG
            } else if constexpr (std::is_same_v<T, int64_t>) {
              return _prefs.getLong64(key, def);

            } else if constexpr (std::is_same_v<T, uint64_t>) {
              return _prefs.getULong64(key, def);
#endif
            } else if constexpr (std::is_same_v<T, int>) {
              return static_cast<int>(_prefs.getInt(key, def));

            } else if constexpr (std::is_same_v<T, float>) {
              return _prefs.getFloat(key, def);
#if WRAPPED_CONFIG_USE_DOUBLE
            } else if constexpr (std::is_same_v<T, double>) {
              return _prefs.getDouble(key, def);
#endif
            } else if constexpr (std::is_same_v<T, LazyString>) {
              return LazyString{_prefs.getString(key, def.c_str())};

            } else {
              return Value::null();
            }
          }, defaultValue);
        }

      protected:
        mutable ::Preferences _prefs;
    };
  }
}
