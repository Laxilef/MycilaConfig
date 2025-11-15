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

      void setWrapper(WrappedConfig* wrapper) override {
        _wrapper = wrapper;
      }

      bool clear() override {
        return true;
      }

      virtual bool exists(const char* key) override {
        const char* pKey = _wrapper ? _wrapper->keyRef(key) : nullptr;
        return pKey && _wrapper->cache().count(pKey);
      }

      virtual bool unset(const char* key) override {
        return exists(key);
      }

      virtual bool set(const char* key, const ValueVariant& variant) override {
        const char* pKey = _wrapper ? _wrapper->keyRef(key) : nullptr;
        if (pKey == nullptr) {
          return false;
        }

        auto it = _wrapper->cache().find(pKey);
        if (it != _wrapper->cache().end()) {
          return it->second != variant;
        }

        return true;
      }

      virtual ValueVariant get(const char* key, const ValueVariant& defaultValue) const override {
        const char* pKey = _wrapper ? _wrapper->keyRef(key) : nullptr;
        if (pKey) {
          auto it = _wrapper->cache().find(pKey);
          if (it != _wrapper->cache().end()) {
            return it->second;
          }
        }

        return emptyVariant;
      }

    private:
      WrappedConfig* _wrapper = nullptr;
  };
} // namespace Mycila
