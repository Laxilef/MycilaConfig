#pragma once

#include "../Types.h"

namespace WrappedConfig {
  class Config;

  namespace Storage {
    class Base {
      public:
        virtual ~Base() {
          end();
        };

        virtual bool begin(const char* name) {
          return true;
        }

        virtual bool end() {
          flush();
          return true;
        }

        virtual void setWrapper(Config*) {}

        virtual bool flush() {
          return false;
        }

        virtual size_t preload() {
          return 0;
        }

        virtual bool clear() = 0;
        virtual bool exists(const char* key) const = 0;
        virtual bool set(const char* key, const Value& value) = 0;
        virtual bool unset(const char* key) = 0;
        virtual Value get(const char* key, const Value& defaultValue) const = 0;
    };
  }
}
