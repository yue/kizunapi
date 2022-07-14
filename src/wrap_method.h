// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_WRAP_METHOD_H_
#define SRC_WRAP_METHOD_H_

#include "src/callback_internal.h"

namespace nb {

template<typename T,
         typename = typename std::enable_if<
             internal::IsFunctionConversionSupported<T>::value>::type>
std::function<napi_value(Arguments*)>
WrapMethod(T&& func, std::function<void(const Arguments&)>&& ref_func) {
  auto holder = internal::CallbackHolderFactory<T>::Create(std::move(func));
  return [holder = std::move(holder),
          ref_func = std::move(ref_func)](Arguments* args) {
    using RunType = typename internal::CallbackHolderFactory<T>::RunType;
    using Runner = internal::ReturnToNode<RunType>;
    bool success = false;
    napi_value ret = Runner::InvokeWithHolder(args, &holder, &success);
    if (success)
      ref_func(*args);
    return ret;
  };
}

}  // namespace nb

#endif  // SRC_WRAP_METHOD_H_
