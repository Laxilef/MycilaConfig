// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once
#include "MycilaStorage.h"

namespace Mycila {
  class DummyStorage : public Storage {
    public:
      bool begin(const char* name) override {
        return true;
      }

      bool clear() override {
        _keys.clear();
        return true;
      }

      virtual bool exists(const char* key) override {
        auto it = std::lower_bound(
          _keys.begin(), _keys.end(), key,
          [](const char* a, const char* b) {
            return strcmp(a, b) < 0;
          }
        );

        return it != _keys.end() && strcmp(*it, key) == 0;
      }

      virtual bool unset(const char* key) override {
        auto it = std::lower_bound(
          _keys.begin(), _keys.end(), key,
          [](const char* a, const char* b) {
            return strcmp(a, b) < 0;
          }
        );

        if (it != _keys.end() && strcmp(*it, key) == 0) {
          _keys.erase(it);
          return true;

        } else {
          return false;
        }
      }

      virtual bool set(const char* key, const ValueVariant& value) override {
        if (!exists(key)) {
          _keys.push_back(key);

          // sort keys
          std::sort(
            _keys.begin(), _keys.end(),
            [](const char* a, const char* b) {
              return strcmp(a, b) < 0;
            }
          );
        }

        return true;
      }

      virtual ValueVariant get(const char* key, const ValueVariant& defaultValue) const override {
        return defaultValue;
      }

    private:
      std::vector<const char*> _keys;
  };
} // namespace Mycila
