#pragma once
#include "../WrappedConfigTypes.h"

namespace WrappedConfig {
  class WrappedConfig;

  namespace Storage {
    class Base {
      public:
        virtual ~Base() { end(); };
        virtual bool begin(const char* name)  = 0;
        virtual bool end() { flush(); return true; }
        virtual void setWrapper(WrappedConfig*) {}
        virtual bool flush() { return false; }
        virtual std::map<const char*, std::string> load() { return {}; }
        virtual bool clear() = 0;
        virtual bool exists(const char* key) = 0;
        virtual bool set(const char* key, const Value& value) = 0;
        virtual bool unset(const char* key) = 0;
        virtual Value get(const char* key, const Value& defaultValue) const = 0;
    };
  }
} // namespace WrappedConfig
