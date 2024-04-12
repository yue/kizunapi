// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_CALLBACK_H_
#define SRC_CALLBACK_H_

#include <memory>

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
std::optional<std::function<ReturnType(ArgTypes...)>>
ConvertWeakFunctionFromNode(
    napi_env env,
    napi_value value,
    int ref_count = 0 /* This parameter is only used internally */) {
  napi_valuetype type;
  napi_status s = napi_typeof(env, value, &type);
  if (s != napi_ok)
    return std::nullopt;
  if (type != napi_function)
    return std::nullopt;
  if (type == napi_null)  // null is accepted as empty function
    return nullptr;
  // Everything bundled with a std::function must be copiable, so we can not
  // simply move the handle here. And we would like to avoid actually copying
  // the Persistent because it requires a handle scope.
  auto handle = std::make_shared<Persistent>(env, value, ref_count);
  return [env, handle](ArgTypes&&... args) -> ReturnType {
    return internal::V8FunctionInvoker<ReturnType(ArgTypes...)>::Go(
        env, handle.get(), std::forward<ArgTypes>(args)...);
  };
}

// Define how callbacks are converted.
template<typename ReturnType, typename... ArgTypes>
struct Type<std::function<ReturnType(ArgTypes...)>> {
  using Sig = ReturnType(ArgTypes...);
  static constexpr const char* name = "Function";
  static inline napi_status ToNode(napi_env env,
                                   std::function<Sig> value,
                                   napi_value* result) {
    if (!value)
      return napi_get_null(env, result);
    return internal::CreateNodeFunction(env, std::move(value), result);
  }
  static inline std::optional<std::function<Sig>> FromNode(
      napi_env env, napi_value value, int ref_count = 1 /* internal */) {
    return ConvertWeakFunctionFromNode<ReturnType, ArgTypes...>(
        env, value, ref_count);
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
