// Copyright (c) zcbenz
// Licensed under the MIT License.

#ifndef SRC_EXCEPTION_H_
#define SRC_EXCEPTION_H_

#include <node_api.h>

#include <sstream>

namespace ki {

inline bool IsExceptionPending(napi_env env) {
  bool result = false;
  napi_is_exception_pending(env, &result);
  return result;
}

inline void ThrowError(napi_env env, const char* message) {
  napi_throw_error(env, nullptr, message);
}

template<typename... ArgTypes>
inline void ThrowError(napi_env env, ArgTypes&&... args) {
  std::ostringstream ss;
  (ss << ... << std::forward<ArgTypes>(args));
  ThrowError(env, ss.str().c_str());
}

inline void ThrowTypeError(napi_env env, const char* message) {
  napi_throw_type_error(env, nullptr, message);
}

template<typename... ArgTypes>
inline void ThrowTypeError(napi_env env, ArgTypes&&... args) {
  std::ostringstream ss;
  (ss << ... << std::forward<ArgTypes>(args));
  ThrowTypeError(env, ss.str().c_str());
}

}  // namespace ki

#endif  // SRC_EXCEPTION_H_
