// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_PROPERTY_H_
#define SRC_PROPERTY_H_

#include <initializer_list>

#include "src/property_internal.h"

namespace nb {

template<typename T>
inline auto Method(T func) {
  return internal::CreatePropertyMethodHolder<
             T, internal::PropertyMethodType::Method>(func);
}

template<typename T>
inline auto Getter(T func) {
  return internal::CreatePropertyMethodHolder<
             T, internal::PropertyMethodType::Getter>(func);
}

template<typename T>
inline auto Setter(T func) {
  return internal::CreatePropertyMethodHolder<
             T, internal::PropertyMethodType::Setter>(func);
}

// Defines a JS property with native methods.
struct Property {
  using Type = internal::PropertyMethodType;

  template<typename... ArgTypes>
  Property(const char* name, ArgTypes... args) : utf8name(name) {
    SetProperty(std::move(args)...);
  }

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

  template<typename Sig>
  void SetProperty(internal::PropertyMethodHolder<Sig, Type::Method> holder) {
    method = WrapPropertyMethod(std::move(holder));
    if (attributes == napi_default)
      attributes = napi_default_method;
  }

  template<typename Sig>
  void SetProperty(internal::PropertyMethodHolder<Sig, Type::Getter> holder) {
    getter = WrapPropertyMethod(std::move(holder));
    if (attributes == napi_default)
      attributes = static_cast<napi_property_attributes>(napi_writable |
                                                         napi_enumerable);
  }

  template<typename Sig>
  void SetProperty(internal::PropertyMethodHolder<Sig, Type::Setter> holder) {
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

// Invoke a property method.
template<internal::PropertyMethodType type>
napi_value InvokePropertyMethod(napi_env env, napi_callback_info info) {
  Arguments args(env, info);
  Property* property = static_cast<Property*>(args.Data());
  using Type = internal::PropertyMethodType;
  if (type == Type::Method)
    return property->method(env, info);
  if (type == Type::Getter)
    return property->getter(env, info);
  if (type == Type::Setter)
    return property->setter(env, info);
}

using PropertyList = std::initializer_list<Property>;

// Define properties on an |object|.
inline bool DefineProperties(napi_env env,
                             napi_value object,
                             PropertyList props) {
  std::vector<napi_property_descriptor> descriptors(props.size());
  for (size_t i = 0; i < props.size(); ++i) {
    // Attach the property holder to object.
    Property* p = new Property(std::move(*(props.begin() + i)));
    napi_status s = napi_add_finalizer(env, object, p,
                                       [](napi_env, void* p, void*) {
      delete static_cast<Property*>(p);
    }, nullptr, nullptr);
    if (s != napi_ok) {
      delete p;
      return false;
    }
    // Translate Property to napi_property_descriptor.
    napi_property_descriptor& d = descriptors[i];
    d.utf8name = p->utf8name;
    d.attributes = p->attributes;
    d.data = p;
    d.value = p->value;
    using Type = internal::PropertyMethodType;
    if (p->method)
      d.method = InvokePropertyMethod<Type::Method>;
    if (p->getter)
      d.getter = InvokePropertyMethod<Type::Getter>;
    if (p->setter)
      d.setter = InvokePropertyMethod<Type::Setter>;
  }
  if (descriptors.size() == 0)
    return true;
  napi_status s = napi_define_properties(env, object, descriptors.size(),
                                         &descriptors.front());
  return s == napi_ok;
}

template<typename... ArgTypes>
inline bool DefineProperties(napi_env env,
                             napi_value object,
                             ArgTypes... props) {
  return DefineProperties(env, object, {props...});
}

}  // namespace nb

#endif  // SRC_PROPERTY_H_
