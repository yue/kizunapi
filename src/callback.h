// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_CALLBACK_H_
#define SRC_CALLBACK_H_

#include "src/callback_internal.h"

namespace nb {

// Create a JS Function that executes a provided C++ function or std::function.
// JavaScript arguments are automatically converted via Type<T>, as is
// the return value of the C++ function, if any.
template<typename Sig>
inline napi_status CreateNodeFunction(napi_env env,
                                      std::function<Sig> value,
                                      napi_value* result,
                                      int flags = 0) {
  using HolderT = internal::CallbackHolder<Sig>;
  return napi_create_function(env, nullptr, 0,
                              &internal::Dispatcher<Sig>::DispatchToCallback,
                              new HolderT(env, std::move(value), flags),
                              result);
}

// Define how callbacks are converted.
template<typename ReturnType, typename... ArgTypes>
struct Type<std::function<ReturnType(ArgTypes...)>> {
  using Sig = ReturnType(ArgTypes...);
  static constexpr const char* name = "Function";
  static inline napi_status ToNode(napi_env env,
                                   std::function<Sig> value,
                                   napi_value* result) {
    return CreateNodeFunction(env, std::move(value), result);
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
    using RunType = typename internal::FunctorTraits<T>::RunType;
    using CallbackType = std::function<RunType>;
    return CreateNodeFunction(env, CallbackType(value), result);
  }
};

// Specialize for member function.
template<typename T>
struct Type<T, typename std::enable_if<
                   std::is_member_function_pointer<T>::value>::type> {
  static constexpr const char* name = "Function";
  static inline napi_status ToNode(napi_env env, T value, napi_value* result) {
    using RunType = typename internal::FunctorTraits<T>::RunType;
    using CallbackType = std::function<RunType>;
    return CreateNodeFunction(env, CallbackType(value), result,
                              HolderIsFirstArgument);
  }
};

}  // namespace nb

#endif  // SRC_CALLBACK_H_
