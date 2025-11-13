// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once
#include "MycilaConfigTypes.h"

namespace Mycila {
  class Storage {
    public:
      virtual ~Storage() { end(); };
      virtual bool begin(const char* name = "config")  = 0;
      virtual bool end() { return true; }
      virtual std::map<const char*, std::string> load(const std::vector<const char*>& keys) { return {}; }
      virtual bool clear() = 0;
      virtual bool exists(const char* key) = 0;
      virtual bool set(const char* key, const ValueVariant& value) = 0;
      virtual bool unset(const char* key) = 0;
      virtual ValueVariant get(const char* key, const ValueVariant& defaultValue) const = 0;
  };
} // namespace Mycila
