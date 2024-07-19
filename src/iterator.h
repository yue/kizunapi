// Copyright (c) zcbenz.
// Licensed under the MIT License.

#ifndef SRC_ITERATOR_H_
#define SRC_ITERATOR_H_

#include "src/types.h"

namespace ki {

template<typename T>
bool IterateArray(napi_env env, napi_value arr,
                  const std::function<bool(uint32_t i, T value)>& visit) {
  if (!IsArray(env, arr))
    return false;
  uint32_t length;
  if (napi_get_array_length(env, arr, &length) != napi_ok)
    return false;
  for (uint32_t i = 0; i < length; ++i) {
    napi_value el = nullptr;
    if (napi_get_element(env, arr, i, &el) != napi_ok)
      return false;
    std::optional<T> out = FromNodeTo<T>(env, el);
    if (!out)
      return false;
    if (!visit(i, *out))
      return false;
  }
  return true;
}

template<typename K, typename V>
bool IterateObject(napi_env env, napi_value obj,
                   const std::function<bool(K key, V value)>& visit) {
  if (!IsType(env, obj, napi_object))
    return false;
  napi_value property_names;
  if (napi_get_property_names(env, obj, &property_names) != napi_ok)
    return false;
  auto visit_arr = [&env, &obj, &visit](uint32_t i, napi_value key) {
    std::optional<K> k = FromNodeTo<K>(env, key);
    if (!k)
      return false;
    napi_value value;
    if (napi_get_property(env, obj, key, &value) != napi_ok)
      return false;
    std::optional<V> v = FromNodeTo<V>(env, value);
    if (!v)
      return false;
    return visit(std::move(*k), std::move(*v));
  };
  return IterateArray<napi_value>(env, property_names, visit_arr);
}

}  // namespace ki

#endif  // SRC_ITERATOR_H_
