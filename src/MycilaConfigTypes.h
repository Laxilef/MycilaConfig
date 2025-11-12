// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <functional>
#include <variant>
#include <string>

namespace Mycila {
  using ValueVariant = std::variant<
    bool,
    int8_t, uint8_t,
    int16_t, uint16_t,
    int32_t, uint32_t,
    int64_t, uint64_t,
    int, float, double,
    std::string
  >;

  typedef std::function<void(const char* key, const ValueVariant& newValue)> ConfigChangeCallback;
  typedef std::function<void()> ConfigRestoredCallback;
  typedef std::function<bool(const char* key, const ValueVariant& newValue)> ConfigValidatorCallback;
} // namespace Mycila
