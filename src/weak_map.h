// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_WEAK_MAP_H_
#define SRC_WEAK_MAP_H_

#include "src/napi_util.h"
#include "src/persistent.h"

namespace nb {

class WeakMap {
 public:
  void Set(napi_env env, napi_value key, napi_value value) {
    napi_value weak_map = Init(env);
    CallMethod(env, weak_map, "set", key, value);
  }

  bool Get(napi_env env, napi_value key, napi_value* result) {
    napi_value weak_map = Init(env);
    napi_value ret = CallMethod(env, weak_map, "get", key);
    if (!ret || !IsObject(env, ret))  // get returns null for unexist key
      return false;
    *result = ret;
    return true;
  }

 private:
  napi_value Init(napi_env env) {
    if (!handle_.IsEmpty())
      return handle_.Get();
    napi_value instance;
    napi_status s = NewInstance(env, &instance);
    assert(s == napi_ok);
    handle_ = Persistent(env, instance);
    return instance;
  }

  static napi_status NewInstance(napi_env env, napi_value* instance) {
    napi_value global;
    napi_status s = napi_get_global(env, &global);
    if (s != napi_ok)
      return s;
    napi_value constructor;
    if (!nb::Get(env, global, "WeakMap", &constructor))
      return napi_generic_failure;
    return napi_new_instance(env, constructor, 0, nullptr, instance);
  }

  Persistent handle_;
};

}  // namespace nb

#endif  // SRC_WEAK_MAP_H_
