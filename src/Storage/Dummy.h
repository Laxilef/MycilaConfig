#pragma once
#include "Base.h"

namespace WrappedConfig {
  namespace Storage {
    class Dummy : public Base {
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
          auto* pItem = _wrapper->getItem(key);
          if (pItem == nullptr) {
            return false;
          }

          return pItem->hasValue();
        }

        virtual bool unset(const char* key) override {
          return exists(key);
        }

        virtual bool set(const char* key, const Value& variant) override {
          auto* pItem = _wrapper->getItem(key);
          if (pItem == nullptr) {
            return false;
          }

          return !pItem->hasValue() || pItem->getValue() != variant;
        }

        virtual Value get(const char* key, const Value& defaultValue) const override {
          auto* pItem = _wrapper->getItem(key);
          if (pItem == nullptr) {
            return Value::null();
          }

          return pItem->getValue();
        }

      private:
        WrappedConfig* _wrapper = nullptr;
    };
  }
} // namespace WrappedConfig
