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
          auto* item = _wrapper->getItem(key);
          if (item == nullptr) {
            return false;
          }

          return item->hasValue();
        }

        virtual bool unset(const char* key) override {
          return exists(key);
        }

        virtual bool set(const char* key, const Value& variant) override {
          auto* item = _wrapper->getItem(key);
          if (item == nullptr) {
            return false;
          }

          return !item->hasValue() || item->getValue() != variant;
        }

        virtual Value get(const char* key, const Value& defaultValue) const override {
          auto* item = _wrapper->getItem(key);
          if (item == nullptr) {
            return Value::null();
          }

          return item->getValue();
        }

      private:
        WrappedConfig* _wrapper = nullptr;
    };
  }
} // namespace WrappedConfig
