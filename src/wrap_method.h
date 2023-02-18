// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_WRAP_METHOD_H_
#define SRC_WRAP_METHOD_H_

#include "src/callback_internal.h"

namespace ki {

namespace internal {

inline void RunRefFunc(const std::function<void(Arguments)>& func,
                       Arguments args,
                       napi_value ret) {
  func(std::move(args));
}

inline void RunRefFunc(const std::function<void(Arguments, napi_value)>& func,
                       Arguments args,
                       napi_value ret) {
  func(std::move(args), ret);
}

}  // namespace internal

template<typename T,
         typename W,
         typename = typename std::enable_if<
             internal::IsFunctionConversionSupported<T>::value>::type>
inline std::function<napi_value(Arguments)>
WrapMethod(T&& func, W&& ref_func) {
  auto holder = internal::CallbackHolderFactory<T>::Create(
      std::move(func), FunctionArgumentIsWeakRef);
  return [holder = std::move(holder),
          ref_func = std::move(ref_func)](Arguments args) {
    using RunType = typename internal::CallbackHolderFactory<T>::RunType;
    using Runner = internal::ReturnToNode<RunType>;
    bool success = false;
    napi_value ret = Runner::InvokeWithHolder(&args, &holder, &success);
    if (success)
      internal::RunRefFunc(ref_func, std::move(args), ret);
    return ret;
  };
}

}  // namespace ki

#endif  // SRC_WRAP_METHOD_H_
