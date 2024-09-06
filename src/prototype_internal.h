// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PROTOTYPE_INTERNAL_H_
#define SRC_PROTOTYPE_INTERNAL_H_

#include "src/property.h"
#include "src/instance_data.h"

namespace ki {

template<typename T>
napi_status ManagePointerInJSWrapper(napi_env env, T* ptr, napi_value* result);

template<typename>
struct AllowPassByValue;

// Check if Type<T> inherits from AllowPassByValue<T>.
template<typename T>
inline constexpr bool can_pass_by_value_v =
    std::is_base_of_v<AllowPassByValue<T>, Type<T>>;

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
    ThrowError(env, "There is no constructor defined.");
  return nullptr;
}

// Check if type has methods defined.
template<typename, typename = void>
struct HasWrap : std::false_type {};

template<typename T>
struct HasWrap<T, std::void_t<decltype(TypeBridge<T>::Wrap)>>
    : std::true_type {};

template<typename, typename = void>
struct HasUnwrap : std::false_type {};

template<typename T>
struct HasUnwrap<T, std::void_t<decltype(TypeBridge<T>::Unwrap)>>
    : std::true_type {};

template<typename T, typename = void>
struct HasFinalize {
  // For class that does not have Finalize defined but inherits from the
  // AllowPassByValue class, we have a default Finalize for them.
  static constexpr bool value = can_pass_by_value_v<T>;
};

template<typename T>
struct HasFinalize<T, std::void_t<decltype(TypeBridge<T>::Finalize)>>
    : std::true_type {};

template<typename, typename = void>
struct HasDestructor : std::false_type {};

template<typename T>
struct HasDestructor<T, std::void_t<decltype(Type<T>::Destructor)>>
    : std::true_type {};

// By default JS object can only be created with "new Class", to allow function
// calls like "Class()", set allow_function_call to true.
template<typename, typename = void>
struct AllowFunctionCall : std::false_type {};

template<typename T>
struct AllowFunctionCall<
    T, std::void_t<decltype(Type<T>::allow_function_call)>> {
  static constexpr bool value = Type<T>::allow_function_call;
};

// Wrap native object to JS according to its type traits.
template<typename T, typename Enable = void>
struct Wrap {
  static inline T* Do(T* ptr) {
    return ptr;
  }
};

template<typename T>
struct Wrap<T, typename std::enable_if<is_function_pointer<
                     decltype(&TypeBridge<T>::Wrap)>::value>::type> {
  static inline auto* Do(T* ptr) {
    auto* ret = TypeBridge<T>::Wrap(ptr);
    static_assert(std::is_same<decltype(ret), T*>::value || HasUnwrap<T>::value,
                  "TypeBridge<T>::Unwrap must be defined if Wrap does not "
                  "return T*");
    return ret;
  }
};

// Unwrap native object from JS according to its type traits.
template<typename T, typename Enable = void>
struct Unwrap {
  static inline T* Do(void* ptr) {
    return static_cast<T*>(ptr);
  }
};

template<typename T>
struct Unwrap<T, typename std::enable_if<is_function_pointer<
                     decltype(&TypeBridge<T>::Unwrap)>::value>::type> {
  static inline T* Do(void* ptr) {
    using WrapReturnType = decltype(TypeBridge<T>::Wrap(nullptr));
    return TypeBridge<T>::Unwrap(static_cast<WrapReturnType>(ptr));
  }
};

// Users can define a Type<T>::AllocateFromNode method to allow converting
// a napi_value to T*, which will be wrapped and stored ina newly allocated JS
// object.
template<typename T, typename Enable = void>
struct AllocateFromNode {
  static inline std::optional<T*> Do(napi_env env, napi_value value) {
    return std::nullopt;
  }
};

template<typename T>
struct AllocateFromNode<
    T, typename std::enable_if<is_function_pointer<
           decltype(&Type<T>::AllocateFromNode)>::value>::type> {
  static inline std::optional<T*> Do(napi_env env, napi_value value) {
    static_assert(HasWrap<T>::value && HasUnwrap<T>::value,
                  "TypeBridge<T>::Wrap/Unwrap must be defined when providing "
                  "the Type<T>::AllocateFromNode method.");
    return Type<T>::AllocateFromNode(env, value);
  }
};

// Called to finalize a JavaScript object.
template<typename T, typename Enable = void>
struct Finalize {
  static inline void Do(void* ptr) {
    // For classes that inherit from AllowPassByValue, use delete by default.
    if constexpr (can_pass_by_value_v<T>) {
      delete static_cast<T*>(ptr);
    }
  }
};

template<typename T>
struct Finalize<T, typename std::enable_if<is_function_pointer<
                     decltype(&TypeBridge<T>::Finalize)>::value>::type> {
  template<typename D>
  static inline void Do(D* ptr) {
    TypeBridge<T>::Finalize(ptr);
  }
};

// Called to destruct a native object.
template<typename T, typename Enable = void>
struct Destruct {
  static inline void Do(void* ptr) {
  }
};

template<typename T>
struct Destruct<T, typename std::enable_if<is_function_pointer<
                     decltype(&Type<T>::Destructor)>::value>::type> {
  static inline void Do(T* ptr) {
    Type<T>::Destructor(ptr);
  }
};

// Receive property list from type and define its prototype.
template<typename T, typename Enable = void>
struct Prototype {
  static inline bool Define(napi_env env, napi_value constructor) {
    return true;
  }
};

template<typename T>
struct Prototype<T, typename std::enable_if<is_function_pointer<
                        decltype(&Type<T>::Define)>::value>::type> {
  static inline bool Define(napi_env env, napi_value constructor) {
    napi_value prototype;
    if (!Get(env, constructor, "prototype", &prototype))
      return false;
    Type<T>::Define(env, constructor, prototype);
    return true;
  }
};

template<typename, typename = void>
struct InheritanceChain;

// Create a new JS object with T's prorotype chain.
template<typename T>
inline napi_value CreateInstance(napi_env env) {
  // Pass an External to indicate it is called from native code.
  napi_value external;
  napi_status s = napi_create_external(env, GetConstructorKey(),
                                       nullptr, nullptr, &external);
  if (s != napi_ok)
    return nullptr;
  // Create a JS object with "new Class(external)".
  napi_value constructor = InheritanceChain<T>::Get(env);
  napi_value object;
  s = napi_new_instance(env, constructor, 1, &external, &object);
  if (s != napi_ok)
    return nullptr;
  return object;
}

// Define T's constructor according to its type traits.
template<typename T, typename Enable = void>
struct DefineClass {
  static napi_status Do(napi_env env, napi_value* result) {
    napi_value constructor;
    napi_status s = napi_define_class(env, Type<T>::name, NAPI_AUTO_LENGTH,
                                      &DummyConstructor, nullptr, 0, nullptr,
                                      &constructor);
    if (s != napi_ok)
      return s;
    // Note that we are not using napi_define_class to set prototype, because
    // it does not support inheritance, check issue below for background.
    // https://github.com/napi-rs/napi-rs/issues/1164
    if (!Prototype<T>::Define(env, constructor))
      return napi_generic_failure;
    *result = constructor;
    return napi_ok;
  }
};

template<typename T>
struct DefineClass<T, typename std::enable_if<is_function_pointer<
                           decltype(&Type<T>::Constructor)>::value>::type> {
  using Sig = typename FunctorTraits<decltype(&Type<T>::Constructor)>::RunType;
  using HolderT = CallbackHolder<Sig>;
  static napi_status Do(napi_env env, napi_value* result) {
    static_assert(HasFinalize<T>::value || HasDestructor<T>::value,
                  "A type that has Type<T>::Constructor defined must also have "
                  "Type<T>::Destructor or TypeBridge<T>::Finalize defined.");
    auto holder = std::make_unique<HolderT>(&Type<T>::Constructor);
    napi_value constructor;
    napi_status s = napi_define_class(env, Type<T>::name, NAPI_AUTO_LENGTH,
                                      &DispatchToCallback, holder.get(),
                                      0, nullptr, &constructor);
    if (s != napi_ok)
      return s;
    if (!Prototype<T>::Define(env, constructor))
      return napi_generic_failure;
    s = AddToFinalizer(env, constructor, std::move(holder));
    if (s != napi_ok)
      return s;
    *result = constructor;
    return napi_ok;
  }
  static napi_value DispatchToCallback(napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    // Only allow constructor call like "new Class()" by default.
    const bool is_constructor_call = args.IsConstructorCall();
    if (!AllowFunctionCall<T>::value && !is_constructor_call) {
      ThrowError(env, "Constructor must be called with new.");
      return nullptr;
    }
    // Let the caller do wrapping if this is called by CreateInstance<T>.
    if (IsCalledFromConverter(args))
      return nullptr;
    // Invoke native constructor.
    std::optional<T*> ptr = CallbackInvoker<Sig>::Invoke(&args);
    if (!ptr || !ptr.value()) {
      ThrowError(env, "Unable to invoke constructor.");
      return nullptr;
    }
    // By default we wrap on the |this| object, unless this is a function call.
    napi_value object = is_constructor_call ? args.This()
                                            : CreateInstance<T>(env);
    // Then wrap the native pointer.
    auto* data = Wrap<T>::Do(ptr.value());
    using DataType = decltype(data);
    napi_ref ref;
    napi_status s = napi_wrap(env, object, data,
                              [](napi_env env, void* data, void* ptr) {
      InstanceData::Get(env)->DeleteWrapper<T>(ptr);
      Finalize<T>::Do(static_cast<DataType>(data));
      Destruct<T>::Do(static_cast<T*>(ptr));
    }, ptr.value(), &ref);
    if (s != napi_ok) {
      Finalize<T>::Do(data);
      Destruct<T>::Do(ptr.value());
      ThrowError(env, "Unable to wrap native object.");
    }
    // Save wrapper.
    InstanceData::Get(env)->AddWrapper<T>(ptr.value(), ref);
    // For constructor call we should never return an object.
    if (is_constructor_call)
      return nullptr;
    else
      return object;
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
template<typename T, typename Enable>
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
                               typename Type<T>::Base>::value>::type> {
  static napi_value Get(napi_env env) {
    napi_value constructor;
    if (!GetOrCreateConstructor<T>(env, &constructor)) {
      // Inherit from base type's constructor.
      napi_value parent = InheritanceChain<typename Type<T>::Base>::Get(env);
      Inherit(env, constructor, parent);
    }
    return constructor;
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

}  // namespace ki

#endif  // SRC_PROTOTYPE_INTERNAL_H_
