// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once
#include <Preferences.h>
#include "MycilaStorage.h"

namespace Mycila {
  class PreferencesStorage : public Storage {
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

      virtual bool exists(const char* key) override {
        return _prefs.isKey(key);
      }

      virtual bool unset(const char* key) override {
        return _prefs.remove(key);
      }

      virtual bool set(const char* key, const ValueVariant& value) override {
        assert(strlen(key) <= 15);
        return std::visit([&](auto&& v) -> bool {
          using T = std::decay_t<decltype(v)>;
          if      constexpr (std::is_same_v<T, bool>)         return _prefs.putBool(key, v);
          else if constexpr (std::is_same_v<T, int8_t>)       return _prefs.putChar(key, static_cast<char>(v));
          else if constexpr (std::is_same_v<T, uint8_t>)      return _prefs.putUChar(key, v);
          else if constexpr (std::is_same_v<T, int16_t>)      return _prefs.putShort(key, static_cast<short>(v));
          else if constexpr (std::is_same_v<T, uint16_t>)     return _prefs.putUShort(key, v);
          else if constexpr (std::is_same_v<T, int32_t>)      return _prefs.putInt(key, v);
          else if constexpr (std::is_same_v<T, uint32_t>)     return _prefs.putUInt(key, v);
          else if constexpr (std::is_same_v<T, int64_t>)      return _prefs.putLong64(key, v);
          else if constexpr (std::is_same_v<T, uint64_t>)     return _prefs.putULong64(key, v);
          else if constexpr (std::is_same_v<T, int>)          return _prefs.putInt(key, v);
          else if constexpr (std::is_same_v<T, float>)        return _prefs.putFloat(key, v);
          else if constexpr (std::is_same_v<T, double>)       return _prefs.putDouble(key, v);
          else if constexpr (std::is_same_v<T, std::string>)  return _prefs.putString(key, v.c_str()) == v.size();
          return false;
        }, value);
      }

      virtual ValueVariant get(const char* key, const ValueVariant& defaultValue) const override {
        return std::visit([&](auto&& def) -> ValueVariant {
          using T = std::decay_t<decltype(def)>;
          if      constexpr (std::is_same_v<T, bool>)         return _prefs.getBool(key, def);
          else if constexpr (std::is_same_v<T, int8_t>)       return static_cast<int8_t>(_prefs.getChar(key, static_cast<char>(def)));
          else if constexpr (std::is_same_v<T, uint8_t>)      return _prefs.getUChar(key, static_cast<unsigned char>(def));
          else if constexpr (std::is_same_v<T, int16_t>)      return static_cast<int16_t>(_prefs.getShort(key, static_cast<short>(def)));
          else if constexpr (std::is_same_v<T, uint16_t>)     return _prefs.getUShort(key, static_cast<unsigned short>(def));
          else if constexpr (std::is_same_v<T, int32_t>)      return _prefs.getInt(key, def);
          else if constexpr (std::is_same_v<T, uint32_t>)     return _prefs.getUInt(key, def);
          else if constexpr (std::is_same_v<T, int64_t>)      return _prefs.getLong64(key, def);
          else if constexpr (std::is_same_v<T, uint64_t>)     return _prefs.getULong64(key, def);
          else if constexpr (std::is_same_v<T, int>)          return static_cast<int>(_prefs.getInt(key, def));
          else if constexpr (std::is_same_v<T, float>)        return _prefs.getFloat(key, def);
          else if constexpr (std::is_same_v<T, double>)       return _prefs.getDouble(key, def);
          else if constexpr (std::is_same_v<T, std::string>)  return std::string{_prefs.getString(key, def.c_str()).c_str()};
          return ValueVariant{};
        }, defaultValue);
      }

    protected:
      mutable Preferences _prefs;
  };
} // namespace Mycila