#pragma once

#define WRAPPED_CONFIG_LOG_TAG "CONFIG"

#ifndef WRAPPED_CONFIG_USE_LONG_LONG
  #define WRAPPED_CONFIG_USE_LONG_LONG 1
#endif

#ifndef WRAPPED_CONFIG_USE_DOUBLE
  #define WRAPPED_CONFIG_USE_DOUBLE 1
#endif

#ifndef WRAPPED_CONFIG_VALUE_TRUE
  #define WRAPPED_CONFIG_VALUE_TRUE "true"
#endif

#ifndef WRAPPED_CONFIG_VALUE_FALSE
  #define WRAPPED_CONFIG_VALUE_FALSE "false"
#endif

#ifndef WRAPPED_CONFIG_EXTENDED_BOOL_VALUE_PARSING
  #define WRAPPED_CONFIG_EXTENDED_BOOL_VALUE_PARSING 1
#endif

// suffix to use for a setting key enabling a feature
#ifndef WRAPPED_CONFIG_KEY_ENABLED_SUFFIX
  #define WRAPPED_CONFIG_KEY_ENABLED_SUFFIX "_enable"
#endif

// suffix to use for a setting key representing a password
#ifndef WRAPPED_CONFIG_KEY_PASSWORD_SUFFIX
  #define WRAPPED_CONFIG_KEY_PASSWORD_SUFFIX "_pwd"
#endif

#ifndef WRAPPED_CONFIG_SHOW_PASSWORD
  #ifndef WRAPPED_CONFIG_PASSWORD_MASK
    #define WRAPPED_CONFIG_PASSWORD_MASK "********"
  #endif
#endif


#include "WrappedConfig/Config.h"