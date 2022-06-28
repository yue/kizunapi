// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef TYPES_H_
#define TYPES_H_

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <node_api.h>

namespace nb {

template<typename T, typename Enable = void>
struct Type {};

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

// Helpers
template<typename T>
inline napi_value ToNode(napi_env env,
                         const T& type,
                         napi_status* status = nullptr) {
  napi_value value = nullptr;
  napi_status s = Type<T>::ToNode(env, type, &value);
  if (s != napi_ok) {
    if (status)
      *status = s;
    // Return undefined on error.
    napi_get_undefined(env, &value);
  }
  return value;
}

template<typename T>
inline bool FromNode(napi_env env, napi_value value, T* out) {
  return Type<T>::FromNode(env, value, out) == napi_ok;
}

}  // namespace nb

#endif  // TYPES_H_
