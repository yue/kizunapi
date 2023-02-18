// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PROTOTYPE_H_
#define SRC_PROTOTYPE_H_

#include "src/prototype_internal.h"

namespace ki {

// Push a constructor to JS.
template<typename T>
struct Class {};

template<typename T>
struct Type<Class<T>> {
  static inline napi_status ToNode(napi_env env,
                                   Class<T>,
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
struct Type<T*, typename std::enable_if<std::is_class<T>::value &&
                                        std::is_class<Type<T>>::value>::type> {
  static constexpr const char* name = Type<T>::name;
  static inline napi_status ToNode(napi_env env, T* ptr, napi_value* result) {
    static_assert(internal::HasWrap<T>::value &&
                  internal::HasFinalize<T>::value,
                  "Converting pointer to JavaScript requires "
                  "TypeBridge<T>::Wrap and TypeBridge<T>::Finalize being "
                  "defined.");
    // Check if there is already a JS object created.
    InstanceData* instance_data = InstanceData::Get(env);
    if (instance_data->GetWeakRef({name, ptr}, result))
      return napi_ok;
    // Pass an External to indicate it is called from native code.
    napi_value external;
    napi_status s = napi_create_external(env, internal::GetConstructorKey(),
                                         nullptr, nullptr, &external);
    if (s != napi_ok)
      return s;
    // Create a JS object with "new Class(external)".
    napi_value constructor = internal::InheritanceChain<T>::Get(env);
    napi_value object;
    s = napi_new_instance(env, constructor, 1, &external, &object);
    if (s != napi_ok)
      return s;
    // Wrap the |ptr| into JS object.
    auto* data = internal::Wrap<T>::Do(ptr);
    using DataType = decltype(data);
    s = napi_wrap(env, object, data, [](napi_env env, void* data, void* ptr) {
      InstanceData::Get(env)->DeleteWeakRef({name, ptr});
      internal::Finalize<T>::Do(static_cast<DataType>(data));
    }, ptr, nullptr);
    if (s != napi_ok) {
      internal::Finalize<T>::Do(data);
      return s;
    }
    // Save weak reference.
    instance_data->AddWeakRef({name, ptr}, object);
    *result = object;
    return napi_ok;
  }
  static inline napi_status FromNode(napi_env env, napi_value value, T** out) {
    void* result;
    napi_status s = napi_unwrap(env, value, &result);
    if (s != napi_ok)
      return s;
    if (!internal::IsInstanceOf<T>(env, value))
      return napi_generic_failure;
    T* ptr = internal::Unwrap<T>::Do(result);
    if (!ptr)
      return napi_generic_failure;
    *out = ptr;
    return napi_ok;
  }
};

}  // namespace ki

#endif  // SRC_PROTOTYPE_H_
