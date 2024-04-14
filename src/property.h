// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PROPERTY_H_
#define SRC_PROPERTY_H_

#include "src/attached_table.h"
#include "src/property_internal.h"

namespace ki {

template<typename T>
inline auto Getter(T func, int flags = 0) {
  return internal::PropertyMethodHolderFactory<
             T, internal::CallbackType::Getter>::Create(func, flags);
}

template<typename T>
inline auto Setter(T func, int flags = 0) {
  return internal::PropertyMethodHolderFactory<
             T, internal::CallbackType::Setter>::Create(func, flags);
}

// Defines a JS property with native methods.
struct Property {
  using Type = internal::CallbackType;

  enum class CacheMode {
    NoCache,
    Getter,
    GetterAndSetter,
  };

  template<typename... ArgTypes>
  Property(std::string name, ArgTypes... args) : name(std::move(name)) {
    SetProperty(std::move(args)...);
    if (attributes == napi_static) {  // napi_static means default here.
      if (value) {
        attributes = napi_default_jsproperty;
        assert(!getter && !setter);
      } else if (getter && setter) {
        attributes = static_cast<napi_property_attributes>(napi_writable |
                                                           napi_enumerable);
        assert(!value);
      } else if (getter) {
        attributes = napi_enumerable;
        assert(!value);
      } else if (setter) {
        attributes = napi_writable;
        assert(!value);
      } else {
        assert(false);
      }
    }
  }

  Property(const Property&) = delete;
  Property(Property&&) = default;

  std::string name;
  std::function<internal::NodeCallbackSig> getter;
  std::function<internal::NodeCallbackSig> setter;
  napi_value value = nullptr;
  CacheMode cache_mode = CacheMode::NoCache;

  // We don't accept napi_static so use it as null.
  napi_property_attributes attributes = napi_static;

 private:
  // Helper to set Property members in arbitrary order.
  void SetProperty(napi_property_attributes attr) {
    assert(!(attr & napi_static));
    attributes = attr;
  }

  void SetProperty(int attr) {
    SetProperty(static_cast<napi_property_attributes>(attr));
  }

  void SetProperty(CacheMode m) {
    cache_mode = m;
  }

  void SetProperty(napi_value v) {
    value = v;
  }

  // Treat raw member object pointer as getter and setter.
  template<typename T>
  typename std::enable_if<std::is_member_object_pointer<T>::value>::type
  SetProperty(T ptr) {
    SetProperty(Getter(ptr), Setter(ptr));
  }

  template<typename Sig>
  void SetProperty(internal::PropertyMethodHolder<Sig, Type::Getter>&& holder) {
    getter = CreateNodeCallbackWithHolder(std::move(holder));
  }

  template<typename Sig>
  void SetProperty(internal::PropertyMethodHolder<Sig, Type::Setter>&& holder) {
    setter = CreateNodeCallbackWithHolder(std::move(holder));
  }

  template<typename T, typename... ArgTypes>
  void SetProperty(T arg, ArgTypes... args) {
    SetProperty(std::move(arg));
    SetProperty(std::move(args)...);
  }
};

namespace internal {

// Invoke a property method.
template<CallbackType type>
napi_value InvokePropertyMethod(napi_env env, napi_callback_info info) {
  Arguments args(env, info);
  Property* property = static_cast<Property*>(args.Data());
  napi_value result;
  if (type == CallbackType::Getter) {
    AttachedTable table;
    if (property->cache_mode == Property::CacheMode::Getter ||
        property->cache_mode == Property::CacheMode::GetterAndSetter) {
      table = AttachedTable(args);
      if (table.Get(property->name, &result))
        return result;
    }
    result = property->getter(env, info);
    if (property->cache_mode == Property::CacheMode::Getter ||
        property->cache_mode == Property::CacheMode::GetterAndSetter)
      table.Set(property->name, result);
  } else if (type == CallbackType::Setter) {
    result = property->setter(env, info);
    if (args.Length() > 0 &&
        property->cache_mode == Property::CacheMode::GetterAndSetter)
      AttachedTable(args).Set(property->name, args[0]);
  }
  return result;
}

// Convert a property to descriptor.
inline napi_property_descriptor PropertyToDescriptor(
    napi_env env, napi_value object, Property prop) {
  // Initialize members to 0.
  napi_property_descriptor descriptor = {};
  // Translate Property to napi_property_descriptor.
  descriptor.name = ToNodeValue(env, prop.name);
  descriptor.attributes = prop.attributes;
  descriptor.value = prop.value;
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

}  // namespace ki

#endif  // SRC_PROPERTY_H_
