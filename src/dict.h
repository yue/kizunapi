// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_DICT_H_
#define SRC_DICT_H_

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

// Helper for getting from Object.
template<typename Key, typename Value>
inline bool Get(napi_env env, napi_value object, Key&& key, Value* out) {
  napi_value v8_key = ToNode(env, std::forward<Key>(key));
  // Check key before get, otherwise this method will always return true for
  // Key == napi_value.
  bool has;
  napi_status s = napi_has_property(env, object, v8_key, &has);
  if (s != napi_ok || !has)
    return false;
  napi_value value;
  s = napi_get_property(env, object, v8_key, &value);
  assert(s == napi_ok);
  return FromNode(env, value, out);
}

// Allow getting arbitrary values.
template<typename Key, typename Value, typename... ArgTypes>
inline bool Get(napi_env env, napi_value object, Key&& key, Value* out,
                ArgTypes&&... args) {
  bool success = Get(env, object, std::forward<Key>(key), out);
  success &= Get(env, object, std::forward<ArgTypes>(args)...);
  return success;
}

// Like Get but ignore unexist keys.
template<typename Key, typename Value>
inline bool ReadOptions(napi_env env, napi_value object,
                        Key&& key, Value* out) {
  napi_value v8_key = ToNode(env, std::forward<Key>(key));
  bool has;
  napi_status s = napi_has_property(env, object, v8_key, &has);
  if (s != napi_ok || !has)
    return true;
  napi_value value;
  s = napi_get_property(env, object, v8_key, &value);
  assert(s == napi_ok);
  return FromNode(env, value, out);
}

template<typename Key, typename Value, typename... ArgTypes>
inline bool ReadOptions(napi_env env, napi_value object, Key&& key, Value* out,
                        ArgTypes&&... args) {
  bool success = ReadOptions(env, object, std::forward<Key>(key), out);
  success &= ReadOptions(env, object, std::forward<ArgTypes>(args)...);
  return success;
}

}  // namespace nb

#endif  // SRC_DICT_H_
