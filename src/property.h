// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PROPERTY_H_
#define SRC_PROPERTY_H_

#include "src/property_internal.h"

namespace nb {

template<typename T>
inline auto Method(T func) {
  return internal::PropertyMethodHolderFactory<
             T, internal::CallbackType::Method>::Create(func);
}

template<typename T>
inline auto Getter(T func) {
  return internal::PropertyMethodHolderFactory<
             T, internal::CallbackType::Getter>::Create(func);
}

template<typename T>
inline auto Setter(T func) {
  return internal::PropertyMethodHolderFactory<
             T, internal::CallbackType::Setter>::Create(func);
}

// Defines a JS property with native methods.
struct Property {
  using Type = internal::CallbackType;

  template<typename... ArgTypes>
  Property(const char* name, ArgTypes... args) : utf8name(name) {
    SetProperty(std::move(args)...);
  }

  Property(const Property&) = delete;
  Property(Property&&) = default;

  const char* utf8name;
  napi_property_attributes attributes = napi_default;
  std::function<internal::NodeCallbackSig> method;
  std::function<internal::NodeCallbackSig> getter;
  std::function<internal::NodeCallbackSig> setter;
  napi_value value = nullptr;

 private:
  // Helper to set Property members in arbitrary order.
  void SetProperty(napi_property_attributes attr) {
    attributes = attr;
  }

  void SetProperty(int attr) {
    attributes = static_cast<napi_property_attributes>(attr);
  }

  void SetProperty(napi_value v) {
    value = v;
    if (attributes == napi_default)
      attributes = napi_default_jsproperty;
  }

  // Treat raw functions as methods.
  template<typename T>
  typename std::enable_if<
      internal::IsFunctionConvertionSupported<T>::value>::type
  SetProperty(T func) {
    return SetProperty(Method(func));
  }

  // Treat raw member object pointer as getter and setter.
  template<typename T>
  typename std::enable_if<std::is_member_object_pointer<T>::value>::type
  SetProperty(T ptr) {
    return SetProperty(Getter(ptr), Setter(ptr));
  }

  template<typename Sig>
  void SetProperty(internal::PropertyMethodHolder<Sig, Type::Method>&& holder) {
    method = WrapPropertyMethod(std::move(holder));
    if (attributes == napi_default)
      attributes = napi_default_method;
  }

  template<typename Sig>
  void SetProperty(internal::PropertyMethodHolder<Sig, Type::Getter>&& holder) {
    getter = WrapPropertyMethod(std::move(holder));
    if (attributes == napi_default)
      attributes = static_cast<napi_property_attributes>(napi_writable |
                                                         napi_enumerable);
  }

  template<typename Sig>
  void SetProperty(internal::PropertyMethodHolder<Sig, Type::Setter>&& holder) {
    setter = WrapPropertyMethod(std::move(holder));
    if (attributes == napi_default)
      attributes = static_cast<napi_property_attributes>(napi_writable |
                                                         napi_enumerable);
  }

  template<typename T, typename... ArgTypes>
  void SetProperty(T arg, ArgTypes... args) {
    SetProperty(std::move(arg));
    SetProperty(std::move(args)...);
  }
};

// Alias Method(name, func) to Property(name, Method(func)).
template<typename T>
inline Property Method(const char* name, T func) {
  return Property(name, Method(func));
}

namespace internal {

// Invoke a property method.
template<CallbackType type>
napi_value InvokePropertyMethod(napi_env env, napi_callback_info info) {
  Arguments args(env, info);
  Property* property = static_cast<Property*>(args.Data());
  if (type == CallbackType::Method)
    return property->method(env, info);
  if (type == CallbackType::Getter)
    return property->getter(env, info);
  if (type == CallbackType::Setter)
    return property->setter(env, info);
}

// Convert a property to descriptor.
inline napi_property_descriptor PropertyToDescriptor(
    napi_env env, napi_value object, Property prop) {
  // Initialize members to 0.
  napi_property_descriptor descriptor = {};
  // Translate Property to napi_property_descriptor.
  descriptor.utf8name = prop.utf8name;
  descriptor.attributes = prop.attributes;
  descriptor.value = prop.value;
  if (prop.method)
    descriptor.method = InvokePropertyMethod<CallbackType::Method>;
  if (prop.getter)
    descriptor.getter = InvokePropertyMethod<CallbackType::Getter>;
  if (prop.setter)
    descriptor.setter = InvokePropertyMethod<CallbackType::Setter>;
  // Attach the property holder to object.
  auto holder = std::make_unique<Property>(std::move(prop));
  descriptor.data = holder.get();
  napi_status s = AddToFinalizer(env, object, std::move(holder));
  if (s != napi_ok)
    return {};
  return descriptor;
}

}  // namespace internal

// Define properties on an |object|.
template<typename... ArgTypes,
         typename = typename std::enable_if<
             internal::is_all_same<ArgTypes..., Property>::value>::type>
inline napi_status DefineProperties(napi_env env, napi_value object,
                                    ArgTypes... props) {
  if (sizeof...(props) == 0)
    return napi_ok;
  std::vector<napi_property_descriptor> desps =
      {internal::PropertyToDescriptor(env, object, std::move(props))...};
  return napi_define_properties(env, object, desps.size(), &desps.front());
}

}  // namespace nb

#endif  // SRC_PROPERTY_H_
