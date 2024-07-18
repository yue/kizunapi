#ifndef SRC_ITERATOR_H_
#define SRC_ITERATOR_H_

#include "src/types.h"

namespace ki {

template<typename T>
bool IterateArray(napi_env env, napi_value arr,
                  const std::function<void(uint32_t i, T value)>& visit) {
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
    visit(i, *out);
  }
  return true;
}

}  // namespace ki

#endif  // SRC_ITERATOR_H_
