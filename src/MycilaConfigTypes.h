// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <functional>
#include <variant>
#include <string>

#include "MycilaLazyString.h"

namespace Mycila {
  using ValueVariant = std::variant<
    std::monostate, // as null
    bool,
    int8_t, uint8_t,
    int16_t, uint16_t,
    int32_t, uint32_t,
    int64_t, uint64_t,
    int, float, double,
    LazyString
  >;

  const ValueVariant emptyVariant{};

  typedef std::function<void(const char* key, const ValueVariant& newValue)> ConfigChangeCallback;
  typedef std::function<void()> ConfigRestoredCallback;
  typedef std::function<bool(const char* key, const ValueVariant& newValue)> ConfigValidatorCallback;
} // namespace Mycila
