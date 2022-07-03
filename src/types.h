// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_TYPES_H_
#define SRC_TYPES_H_

#include <vector>
#include <string>

#include <assert.h>
#include <node_api.h>

#include "src/template_util.h"

namespace nb {

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
  static napi_status FromNode(napi_env env, napi_value value, napi_value* out) {
    *out = value;
    return napi_ok;
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
struct Type<int32_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   int32_t value,
                                   napi_value* result) {
    return napi_create_int32(env, value, result);
  }
  static napi_status FromNode(napi_env env, napi_value value, int32_t* out) {
    return napi_get_value_int32(env, value, out);
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
  static napi_status FromNode(napi_env env, napi_value value, uint32_t* out) {
    return napi_get_value_uint32(env, value, out);
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
  static napi_status FromNode(napi_env env, napi_value value, int64_t* out) {
    return napi_get_value_int64(env, value, out);
  }
};

template<>
struct Type<float> {
  static constexpr const char* name = "Number";
  static inline napi_status ToNode(napi_env env,
                                   float value,
                                   napi_value* result) {
    return napi_create_double(env, value, result);
  }
  static napi_status FromNode(napi_env env, napi_value value, float* out) {
    double intermediate;
    napi_status s = napi_get_value_double(env, value, &intermediate);
    if (s == napi_ok)
      *out = intermediate;
    return s;
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
  static napi_status FromNode(napi_env env, napi_value value, double* out) {
    return napi_get_value_double(env, value, out);
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
  static napi_status FromNode(napi_env env, napi_value value, bool* out) {
    return napi_get_value_bool(env, value, out);
  }
};

template<>
struct Type<std::string> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const std::string& value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value.c_str(), value.length(), result);
  }
  static napi_status FromNode(napi_env env,
                              napi_value value,
                              std::string* out) {
    size_t length;
    napi_status s = napi_get_value_string_utf8(env, value, nullptr, 0, &length);
    if (s != napi_ok)
      return s;
    if (length > 0) {
      out->reserve(length + 1);
      out->resize(length);
      return napi_get_value_string_utf8(env, value, &out->front(),
                                        out->capacity(), nullptr);
    } else {
      out->clear();
      return napi_ok;
    }
  }
};

template<>
struct Type<std::u16string> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const std::u16string& value,
                                   napi_value* result) {
    return napi_create_string_utf16(env, value.c_str(), value.length(), result);
  }
  static napi_status FromNode(napi_env env,
                              napi_value value,
                              std::u16string* out) {
    size_t length;
    napi_status s = napi_get_value_string_utf16(env, value, nullptr, 0, &length);
    if (s != napi_ok)
      return s;
    if (length > 0) {
      out->reserve(length + 1);
      out->resize(length);
      return napi_get_value_string_utf16(env, value, &out->front(),
                                         out->capacity(), nullptr);
    } else {
      out->clear();
      return napi_ok;
    }
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

// Helpers
template<typename T>
inline napi_value ToNode(napi_env env, const T& value) {
  napi_value result = nullptr;
  napi_status s = Type<T>::ToNode(env, value, &result);
  if (s != napi_ok) {
    // Return undefined on error.
    s = napi_get_undefined(env, &result);
    assert(s == napi_ok);
  }
  return result;
}

template<typename T>
inline bool FromNode(napi_env env, napi_value value, T* out) {
  return Type<T>::FromNode(env, value, out) == napi_ok;
}

}  // namespace nb

#endif  // SRC_TYPES_H_
