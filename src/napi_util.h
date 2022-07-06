// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_NAPI_UTIL_H_
#define SRC_NAPI_UTIL_H_

#include <node_api.h>

#include <memory>

namespace nb {

// Free the |ptr| in |object|'s finalizer.
template<typename T>
napi_status AddToFinalizer(napi_env env, napi_value object,
                           std::unique_ptr<T> ptr) {
  napi_status s = napi_add_finalizer(env, object, ptr.get(),
                                     [](napi_env, void* ptr, void*) {
    delete static_cast<T*>(ptr);
  }, nullptr, nullptr);
  if (s != napi_ok)
    return s;
  ptr.release();
  return napi_ok;
}

}  // namespace nb

#endif  // SRC_NAPI_UTIL_H_
