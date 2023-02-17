// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_CALLBACK_H_
#define SRC_CALLBACK_H_

#include "src/callback_internal.h"

namespace ki {

// This API only holds weak references to the JS callback, because V8 can
// not resolve circular reference between C++ and JavaScript, and it is easy
// for a callback's closure to reference the callback's owner.
// Consider this code:
//   const win = new Window()
//   win.onClick = () => win.close()
// The `onClick` callback holds a strong reference of `win`, if we store the
// callback with a strong Persistent in `win`'s C++ class, then `win` will
// never get a chance to be garbage collected.
// The solution is to store `onClick` in C++ with a weak reference, and then
// store the strong reference of it in JavaScript with code like:
//   weakMap.get(win).set('onClick', callback)
// In this way V8 can recognize the circular reference and do GC.
template<typename ReturnType, typename... ArgTypes>
napi_status WeakFunctionFromNode(napi_env env,
                                 napi_value value,
                                 std::function<ReturnType(ArgTypes...)>* out,
                                 // This parameter is only used internally.
                                 int ref_count = 0) {
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
  Persistent handle(env, value, ref_count);
  *out = [env, handle](ArgTypes&&... args) -> ReturnType {
    return internal::V8FunctionInvoker<ReturnType(ArgTypes...)>::Go(
        env, handle, std::forward<ArgTypes>(args)...);
  };
  return napi_ok;
}

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
    return WeakFunctionFromNode(env, value, out, 1 /* ref_count */);
  }
};

template<typename T>
struct Type<T, typename std::enable_if<
                   internal::IsFunctionConversionSupported<T>::value>::type> {
  static constexpr const char* name = "Function";
  static inline napi_status ToNode(napi_env env, T value, napi_value* result) {
    return internal::CreateNodeFunction(env, value, result);
  }
};

}  // namespace ki

#endif  // SRC_CALLBACK_H_
