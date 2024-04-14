// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_DICT_H_
#define SRC_DICT_H_

#include "src/types.h"

namespace ki {

namespace internal {

// Node throws if non-object is passed to property APIs.
inline bool IsObject(napi_env env, napi_value value) {
  napi_valuetype type;
  return napi_typeof(env, value, &type) == napi_ok &&
         (type == napi_object || type == napi_function);
}

}  // namespace internal

// Helper for setting Object.
template<typename Key, typename Value>
inline bool Set(napi_env env, napi_value object, Key&& key, Value&& value) {
  if (!internal::IsObject(env, object))
    return false;
  return napi_ok == napi_set_property(
      env, object,
      ki::ToNodeValue(env, std::forward<Key>(key)),
      ki::ToNodeValue(env, std::forward<Value>(value)));
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
  if (!internal::IsObject(env, object))
    return false;
  napi_value v8_key = ToNodeValue(env, std::forward<Key>(key));
  // Check key before get, otherwise this method will always return true for
  // Key == napi_value.
  bool has;
  napi_status s = napi_has_property(env, object, v8_key, &has);
  if (s != napi_ok || !has)
    return false;
  napi_value value;
  s = napi_get_property(env, object, v8_key, &value);
  assert(s == napi_ok);
  std::optional<Value> result = FromNodeTo<Value>(env, value);
  if (!result)
    return false;
  *out = std::move(*result);
  return true;
}

// Allow getting arbitrary values.
template<typename Key, typename Value, typename... ArgTypes>
inline bool Get(napi_env env, napi_value object, Key&& key, Value* out,
                ArgTypes&&... args) {
  bool success = Get(env, object, std::forward<Key>(key), out);
  success &= Get(env, object, std::forward<ArgTypes>(args)...);
  return success;
}

// Remove proeprties.
template<typename Key>
inline bool Delete(napi_env env, napi_value object, Key&& key) {
  if (!internal::IsObject(env, object))
    return false;
  bool success;
  napi_status s = napi_delete_property(
      env, object, ToNodeValue(env, std::forward<Key>(key)), &success);
  return s == napi_ok && success;
}

// Like Get but ignore unexist keys.
template<typename Key, typename Value>
inline bool ReadOptions(napi_env env, napi_value object,
                        Key&& key, Value* out) {
  if (!internal::IsObject(env, object))
    return false;
  napi_value v8_key = ToNodeValue(env, std::forward<Key>(key));
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

}  // namespace ki

#endif  // SRC_DICT_H_
