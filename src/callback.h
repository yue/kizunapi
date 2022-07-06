// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_CALLBACK_H_
#define SRC_CALLBACK_H_

#include "src/callback_internal.h"

namespace nb {

// Define how callbacks are converted.
template<typename ReturnType, typename... ArgTypes>
struct Type<std::function<ReturnType(ArgTypes...)>> {
  using Sig = ReturnType(ArgTypes...);
  static constexpr const char* name = "Function";
  static inline napi_status ToNode(napi_env env,
                                   std::function<Sig> value,
                                   napi_value* result) {
    return internal::CreateNodeFunction(env, std::move(value), result);
  }
  static napi_status FromNode(napi_env env,
                              napi_value value,
                              std::function<Sig>* out) {
    napi_valuetype type;
    napi_status s = napi_typeof(env, value, &type);
    if (s != napi_ok)
      return s;
    if (type == napi_null) {
      *out = nullptr;
      return napi_ok;
    }
    if (type != napi_function)
      return napi_function_expected;
    Persistent handle(env, value);
    *out = [env, handle](ArgTypes&&... args) -> ReturnType {
      return internal::V8FunctionInvoker<ReturnType(ArgTypes...)>::Go(
          env, handle, std::forward<ArgTypes>(args)...);
    };
    return napi_ok;
  }
};

// Specialize for native functions.
template<typename T>
struct Type<T, typename std::enable_if<
                   internal::is_function_pointer<T>::value>::type> {
  static constexpr const char* name = "Function";
  static inline napi_status ToNode(napi_env env, T value, napi_value* result) {
    return internal::CreateNodeFunction(env, value, result);
  }
};

// Specialize for member function.
template<typename T>
struct Type<T, typename std::enable_if<
                   std::is_member_function_pointer<T>::value>::type> {
  static constexpr const char* name = "Function";
  static inline napi_status ToNode(napi_env env, T value, napi_value* result) {
    return internal::CreateNodeFunction(env, value, result);
  }
};

}  // namespace nb

#endif  // SRC_CALLBACK_H_
