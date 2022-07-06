// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PROTOTYPE_H_
#define SRC_PROTOTYPE_H_

#include "src/prototype_internal.h"

namespace nb {

// Push a constructor to JS.
template<typename T>
struct Constructor {};

template<typename T>
struct Type<Constructor<T>> {
  static inline napi_status ToNode(napi_env env,
                                   Constructor<T>,
                                   napi_value* result) {
    napi_value constructor = internal::InheritanceChain<T>::Get(env);
    if (!constructor)
      return napi_generic_failure;
    *result = constructor;
    return napi_ok;
  }
};

// Default converter for pointers.
template<typename T>
struct Type<T*, typename std::enable_if<std::is_class<T>::value>::type> {
  static constexpr const char* name = Type<T>::name;
  static inline napi_status ToNode(napi_env env, T* ptr, napi_value* result) {
    // Check if there is already a JS object created.
    InstanceData* instance_data = InstanceData::Get(env);
    if (instance_data->Get(ptr, result))
      return napi_ok;
    // Pass an External to indicate it is called from native code.
    napi_value external;
    napi_status s = napi_create_external(env, internal::GetConstructorKey(),
                                         nullptr, nullptr, &external);
    if (s != napi_ok)
      return s;
    // Create a JS object with "new Constructor(external)".
    napi_value constructor = internal::InheritanceChain<T>::Get(env);
    napi_value object;
    s = napi_new_instance(env, constructor, 1, &external, &object);
    if (s != napi_ok)
      return s;
    // Wrap the |ptr| into JS object.
    void* data = Type<T>::Wrap(ptr);
    s = napi_wrap(env, object, data, [](napi_env env, void* data, void* ptr) {
      InstanceData::Get(env)->Remove(ptr);
      Type<T>::Finalize(data);
    }, ptr, nullptr);
    if (s != napi_ok) {
      Type<T>::Finalize(data);
      return s;
    }
    *result = object;
    // Save weak reference.
    instance_data->Set(ptr, object).MakeWeak();
    return napi_ok;
  }
  static inline napi_status FromNode(napi_env env, napi_value value, T** out) {
    void* result;
    napi_status s = napi_unwrap(env, value, &result);
    if (s != napi_ok)
      return s;
    if (!internal::IsInstanceOf<T>(env, value))
      return napi_generic_failure;
    return internal::Unwrap<T>::Do(result, out) ? napi_ok
                                                : napi_generic_failure;
  }
};

}  // namespace nb

#endif  // SRC_PROTOTYPE_H_
