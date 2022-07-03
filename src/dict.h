// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_DICT_H_
#define SRC_DICT_H_

#include <utility>

#include "src/types.h"

namespace nb {

inline napi_value CreateObject(napi_env env) {
  napi_value value = nullptr;
  napi_status s = napi_create_object(env, &value);
  assert(s == napi_ok);
  return value;
}

// Helper for setting Object.
template<typename Key, typename Value>
inline bool Set(napi_env env, napi_value object, Key&& key, Value&& value) {
  return napi_set_property(env, object,
                          nb::ToNode(env, std::forward<Key>(key)),
                          nb::ToNode(env, std::forward<Value>(value)));
}

// Allow setting arbitrary key/value pairs.
template<typename Key, typename Value, typename... ArgTypes>
inline bool Set(napi_env env, napi_value object, Key&& key, Value&& value,
                ArgTypes&&... args) {
  bool r = Set(env, object, std::forward<Key>(key), std::forward<Value>(value));
  r &= Set(env, object, std::forward<ArgTypes>(args)...);
  return r;
}

}  // namespace nb

#endif  // SRC_DICT_H_
