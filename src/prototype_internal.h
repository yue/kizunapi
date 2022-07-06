// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_PROTOTYPE_INTERNAL_H_
#define SRC_PROTOTYPE_INTERNAL_H_

#include "src/property.h"
#include "src/instance_data.h"

namespace nb {

namespace internal {

// Return a key used to indicate whether the constructor should be called.
inline void* GetConstructorKey() {
  static int key = 0x8964;
  return &key;
}

// Check if we should skip calling native constructor.
inline bool IsCalledFromConverter(const Arguments& args) {
  // Return if first arg is external.
  void* key;
  return args.Length() == 1 &&
         napi_get_value_external(args.Env(), args[0], &key) == napi_ok &&
         key == GetConstructorKey();
}

// The default constructor.
inline napi_value DummyConstructor(napi_env env, napi_callback_info info) {
  if (!IsCalledFromConverter(Arguments(env, info)))
    napi_throw_error(env, nullptr, "There is no constructor defined.");
  return nullptr;
}

// Define T's constructor according to its type traits.
template<typename T, typename Enable = void>
struct DefineClass {
  static napi_status Do(napi_env env, napi_value* prototype) {
    return napi_define_class(env, Type<T>::name, NAPI_AUTO_LENGTH,
                             &DummyConstructor, nullptr, 0, nullptr,
                             prototype);
  }
};


template<typename T>
struct DefineClass<T, typename std::enable_if<is_function_pointer<
                           decltype(&Type<T>::Constructor)>::value>::type> {
  using Sig = typename FunctorTraits<decltype(&Type<T>::Constructor)>::RunType;
  static napi_status Do(napi_env env, napi_value* prototype) {
    return napi_define_class(
        env, Type<T>::name, NAPI_AUTO_LENGTH, &DispatchToCallback,
        new CallbackHolder<Sig>{&Type<T>::Constructor},
        0, nullptr, prototype);
  }
  static napi_value DispatchToCallback(napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    if (!args.IsConstructorCall()) {
      napi_throw_error(env, nullptr, "Constructor must be called with new.");
      return nullptr;
    }
    // If we should let the caller do wrapping.
    if (IsCalledFromConverter(args))
      return nullptr;
    // Invoke native constructor.
    T* ptr = CFunctionInvoker<Sig>::Invoke(&args);
    if (!ptr) {
      napi_throw_error(env, nullptr, "Unable to invoke constructor.");
      return nullptr;
    }
    // Then wrap the native pointer.
    napi_status s = napi_wrap(env, args.GetThis(), ptr,
                              [](napi_env, void* ptr, void*) {
      Type<T>::Destructor(static_cast<T*>(ptr));
    }, nullptr, nullptr);
    if (s != napi_ok) {
      Type<T>::Destructor(ptr);
      napi_throw_error(env, nullptr, "Unable to wrap native object.");
    }
    // Save weak reference.
    InstanceData::Get(env)->Set(ptr, args.GetThis()).MakeWeak();
    return nullptr;
  }
};

// Get bare constructor for T.
template<typename T>
bool GetOrCreateConstructor(napi_env env, napi_value* constructor) {
  // Get cached constructor.
  static int key = 0x19980604;
  InstanceData* instance_data = InstanceData::Get(env);
  if (instance_data->Get(&key, constructor))
    return true;
  // Create a new one if not found.
  napi_status s = DefineClass<T>::Do(env, constructor);
  assert(s == napi_ok);
  // Cache it forever.
  instance_data->Set(&key, *constructor);
  return false;
}

// Implement inheritance with setPrototypeOf due to lack of native napi.
inline void Inherit(napi_env env, napi_value child, napi_value parent) {
  napi_value global, object, set_prototype_of, child_proto, parent_proto;
  if (!(napi_get_global(env, &global) == napi_ok &&
        Get(env, global, "Object", &object) &&
        Get(env, object, "setPrototypeOf", &set_prototype_of) &&
        Get(env, child, "prototype", &child_proto) &&
        Get(env, parent, "prototype", &parent_proto))) {
    assert(false);
    return;
  }
  // Object.setPrototypeOf(Child, Parent.prototype)
  napi_value args1[] = {child_proto, parent_proto};
  napi_status s = napi_call_function(env, object, set_prototype_of, 2, args1,
                                     nullptr);
  assert(s == napi_ok);
  // Object.setPrototypeOf(Child, Parent)
  napi_value args2[] = {child, parent};
  s = napi_call_function(env, object, set_prototype_of, 2, args2, nullptr);
  assert(s == napi_ok);
}

// Get constructor with populated prototype for T.
template<typename T, typename Enable = void>
struct InheritanceChain {
  // There is no base type.
  static napi_value Get(napi_env env) {
    napi_value constructor = nullptr;
    GetOrCreateConstructor<T>(env, &constructor);
    return constructor;
  }
};

template<typename T>
struct InheritanceChain<T, typename std::enable_if<std::is_class<
                               typename Type<T>::base>::value>::type> {
  static napi_value Get(napi_env env) {
    napi_value constructor;
    if (!GetOrCreateConstructor<T>(env, &constructor)) {
      // Inherit from base type's constructor.
      napi_value parent = InheritanceChain<typename Type<T>::base>::Get(env);
      Inherit(env, constructor, parent);
    }
    return constructor;
  }
};

// Unwrap native object from JS according to its type traits.
template<typename T, typename Enable = void>
struct Unwrap {
  static inline bool Do(void* ptr, T** out) {
    *out = static_cast<T*>(ptr);
    return true;
  }
};

template<typename T>
struct Unwrap<T, typename std::enable_if<is_function_pointer<
                     decltype(&Type<T>::Unwrap)>::value>::type> {
  static inline bool Do(void* ptr, T** out) {
    return Type<T>::Unwrap(ptr, out);
  }
};

// Return if JS |object| is an instance of |T|.
template<typename T>
bool IsInstanceOf(napi_env env, napi_value object) {
  napi_value constructor;
  if (!GetOrCreateConstructor<T>(env, &constructor))
    return false;
  bool result = false;
  napi_instanceof(env, object, constructor, &result);
  // TODO(zcbenz): Check native type tag.
  return result;
}

}  // namespace internal

}  // namespace nb

#endif  // SRC_PROTOTYPE_INTERNAL_H_
