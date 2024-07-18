// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_TYPES_H_
#define SRC_TYPES_H_

#include <assert.h>
#include <node_api.h>

#include <cstring>
#include <optional>

#include "src/template_util.h"

namespace ki {

template<typename T, typename Enable = void>
struct TypeBridge {};

template<typename T, typename Enable = void>
struct Type {};

template<>
struct Type<napi_value> {
  static constexpr const char* name = "Value";
  static inline napi_status ToNode(napi_env env,
                                   napi_value value,
                                   napi_value* result) {
    *result = value;
    return napi_ok;
  }
  static inline std::optional<napi_value> FromNode(napi_env env,
                                                   napi_value value) {
    return value;
  }
};

template<>
struct Type<std::nullptr_t> {
  static constexpr const char* name = "Null";
  static inline napi_status ToNode(napi_env env,
                                   std::nullptr_t value,
                                   napi_value* result) {
    return napi_get_null(env, result);
  }
};

template<>
struct Type<void*> {
  static constexpr const char* name = "Buffer";
  static inline napi_status ToNode(napi_env env,
                                   void* value,
                                   napi_value* result) {
    void* data;
    napi_status s = napi_create_buffer(env, sizeof(void*), &data, result);
    if (s != napi_ok)
      return s;
    std::memcpy(data, &value, sizeof(void*));
    return napi_ok;
  }
};

template<>
struct Type<uint8_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   uint8_t value,
                                   napi_value* result) {
    return napi_create_uint32(env, value, result);
  }
  static inline std::optional<uint8_t> FromNode(napi_env env,
                                                napi_value value) {
    uint32_t val;
    if (napi_get_value_uint32(env, value, &val) != napi_ok)
      return std::nullopt;
    return static_cast<uint8_t>(val);
  }
};

template<>
struct Type<uint16_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   uint16_t value,
                                   napi_value* result) {
    return napi_create_uint32(env, value, result);
  }
  static inline std::optional<uint16_t> FromNode(napi_env env,
                                                 napi_value value) {
    uint32_t val;
    if (napi_get_value_uint32(env, value, &val) != napi_ok)
      return std::nullopt;
    return static_cast<uint16_t>(val);
  }
};

template<>
struct Type<int8_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   int8_t value,
                                   napi_value* result) {
    return napi_create_int32(env, value, result);
  }
  static inline std::optional<int8_t> FromNode(napi_env env, napi_value value) {
    int32_t val;
    if (napi_get_value_int32(env, value, &val) != napi_ok)
      return std::nullopt;
    return static_cast<int8_t>(val);
  }
};

template<>
struct Type<int16_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   int16_t value,
                                   napi_value* result) {
    return napi_create_int32(env, value, result);
  }
  static inline std::optional<int16_t> FromNode(napi_env env,
                                                napi_value value) {
    int32_t val;
    if (napi_get_value_int32(env, value, &val) != napi_ok)
      return std::nullopt;
    return static_cast<int16_t>(val);
  }
};

template<>
struct Type<int32_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   int32_t value,
                                   napi_value* result) {
    return napi_create_int32(env, value, result);
  }
  static inline std::optional<int32_t> FromNode(napi_env env,
                                                napi_value value) {
    int32_t result;
    if (napi_get_value_int32(env, value, &result) == napi_ok)
      return result;
    return std::nullopt;
  }
};

template<>
struct Type<uint32_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   uint32_t value,
                                   napi_value* result) {
    return napi_create_uint32(env, value, result);
  }
  static inline std::optional<uint32_t> FromNode(napi_env env,
                                                 napi_value value) {
    uint32_t result;
    if (napi_get_value_uint32(env, value, &result) == napi_ok)
      return result;
    return std::nullopt;
  }
};

template<>
struct Type<int64_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   int64_t value,
                                   napi_value* result) {
    return napi_create_int64(env, value, result);
  }
  static inline std::optional<int64_t> FromNode(napi_env env,
                                                napi_value value) {
    int64_t result;
    if (napi_get_value_int64(env, value, &result) == napi_ok)
      return result;
    return std::nullopt;
  }
};

#if defined(__APPLE__)
template<>
struct Type<size_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   size_t value,
                                   napi_value* result) {
    return napi_create_int64(env, value, result);
  }
  static inline std::optional<size_t> FromNode(napi_env env,
                                               napi_value value) {
    int64_t result;
    if (napi_get_value_int64(env, value, &result) == napi_ok)
      return result;
    return std::nullopt;
  }
};
#endif

template<>
struct Type<float> {
  static constexpr const char* name = "Number";
  static inline napi_status ToNode(napi_env env,
                                   float value,
                                   napi_value* result) {
    return napi_create_double(env, value, result);
  }
  static inline std::optional<float> FromNode(napi_env env,
                                              napi_value value) {
    double intermediate;
    if (napi_get_value_double(env, value, &intermediate) == napi_ok)
      return static_cast<float>(intermediate);
    return std::nullopt;
  }
};

template<>
struct Type<double> {
  static constexpr const char* name = "Number";
  static inline napi_status ToNode(napi_env env,
                                   double value,
                                   napi_value* result) {
    return napi_create_double(env, value, result);
  }
  static inline std::optional<double> FromNode(napi_env env,
                                               napi_value value) {
    double result;
    if (napi_get_value_double(env, value, &result) == napi_ok)
      return result;
    return std::nullopt;
  }
};

template<>
struct Type<uint64_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   uint64_t value,
                                   napi_value* result) {
    return Type<double>::ToNode(env, static_cast<double>(value), result);
  }
  static inline std::optional<uint64_t> FromNode(napi_env env,
                                                 napi_value value) {
    auto result = Type<double>::FromNode(env, value);
    if (!result)
      return std::nullopt;
    return static_cast<uint64_t>(*result);
  }
};

template<>
struct Type<bool> {
  static constexpr const char* name = "Boolean";
  static inline napi_status ToNode(napi_env env,
                                   bool value,
                                   napi_value* result) {
    return napi_get_boolean(env, value, result);
  }
  static inline std::optional<bool> FromNode(napi_env env,
                                             napi_value value) {
    bool result;
    if (napi_get_value_bool(env, value, &result) == napi_ok)
      return result;
    return std::nullopt;
  }
};

template<>
struct Type<const char*> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char* value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, result);
  }
};

template<>
struct Type<char*> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char* value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, result);
  }
};

template<size_t n>
struct Type<char[n]> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char* value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value, n - 1, result);
  }
};

template<>
struct Type<const char16_t*> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char16_t* value,
                                   napi_value* result) {
    return napi_create_string_utf16(env, value, NAPI_AUTO_LENGTH, result);
  }
};

template<>
struct Type<char16_t*> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char16_t* value,
                                   napi_value* result) {
    return napi_create_string_utf16(env, value, NAPI_AUTO_LENGTH, result);
  }
};

template<size_t n>
struct Type<char16_t[n]> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char16_t* value,
                                   napi_value* result) {
    return napi_create_string_utf16(env, value, n - 1, result);
  }
};

template<>
struct Type<napi_env> {
  static constexpr const char* name = "Environment";
};

// Optimized helper for creating symbols.
struct SymbolHolder {
  bool symbol_for;
  const char* str;
};

inline SymbolHolder Symbol(const char* str) {
  return {false, str};
}

inline SymbolHolder SymbolFor(const char* str) {
  return {true, str};
}

template<>
struct Type<SymbolHolder> {
  static constexpr const char* name = "Symbol";
  static inline napi_status ToNode(napi_env env,
                                   SymbolHolder value,
                                   napi_value* result) {
    napi_value symbol;
    if (value.symbol_for) {
      return node_api_symbol_for(env, value.str, NAPI_AUTO_LENGTH, result);
    } else {
      napi_status s = Type<const char*>::ToNode(env, value.str, &symbol);
      if (s != napi_ok)
        return s;
      return napi_create_symbol(env, symbol, result);
    }
  }
};

// Some builtin types.
inline napi_value Global(napi_env env) {
  napi_value result;
  napi_status s = napi_get_global(env, &result);
  assert(s == napi_ok);
  return result;
}

inline napi_value Undefined(napi_env env) {
  napi_value result;
  napi_status s = napi_get_undefined(env, &result);
  assert(s == napi_ok);
  return result;
}

inline napi_value Null(napi_env env) {
  napi_value result;
  napi_status s = napi_get_null(env, &result);
  assert(s == napi_ok);
  return result;
}

// Object helpers.
inline napi_value CreateObject(napi_env env) {
  napi_value value = nullptr;
  napi_status s = napi_create_object(env, &value);
  assert(s == napi_ok);
  return value;
}

inline bool IsArray(napi_env env, napi_value value) {
  bool result = false;
  napi_is_array(env, value, &result);
  return result;
}

inline bool IsType(napi_env env, napi_value value, napi_valuetype target) {
  napi_valuetype type;
  napi_status s = napi_typeof(env, value, &type);
  return s == napi_ok && type == target;
}

// Function helpers.
template<typename T>
inline napi_status ConvertToNode(napi_env env, T&& value,
                                 napi_value* result) {
  return Type<std::decay_t<T>>::ToNode(env, std::forward<T>(value), result);
}

template<typename In>
inline napi_value ToNodeValue(napi_env env, In&& value) {
  napi_value result = nullptr;
  napi_status s = ConvertToNode(env, std::forward<In>(value), &result);
  // Return undefined on error.
  if (s != napi_ok)
    return Undefined(env);
  return result;
}

// The short version of FromNode.
template<typename T>
inline std::optional<T> FromNodeTo(napi_env env, napi_value value) {
  return Type<T>::FromNode(env, value);
}

}  // namespace ki

#endif  // SRC_TYPES_H_
