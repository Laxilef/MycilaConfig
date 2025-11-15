// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once
#include "MycilaConfigTypes.h"

namespace Mycila {
  class WrappedConfig;
  class Storage {
    public:
      virtual ~Storage() { end(); };
      virtual bool begin(const char* name = "config")  = 0;
      virtual bool end() { flush(); return true; }
      virtual void setWrapper(WrappedConfig*) {}
      virtual bool flush() { return false; }
      virtual std::map<const char*, std::string> load() { return {}; }
      virtual bool clear() = 0;
      virtual bool exists(const char* key) = 0;
      virtual bool set(const char* key, const ValueVariant& value) = 0;
      virtual bool unset(const char* key) = 0;
      virtual ValueVariant get(const char* key, const ValueVariant& defaultValue) const = 0;
  };
} // namespace Mycila
