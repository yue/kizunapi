// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PROPERTY_INTERNAL_H_
#define SRC_PROPERTY_INTERNAL_H_

#include "src/callback_internal.h"

namespace nb {

namespace internal {

using NodeCallbackSig = napi_value(napi_env, napi_callback_info);

// Add type tag for the CallbackHolder.
enum class PropertyMethodType {
  Method,
  Getter,
  Setter,
};

template<typename Sig, PropertyMethodType type>
struct PropertyMethodHolder : CallbackHolder<Sig> {
  explicit PropertyMethodHolder(CallbackHolder<Sig> holder)
      : CallbackHolder<Sig>(std::move(holder)) {}
};

// Create a std::function<napi_callback> from the passed |holder|, that can be
// executed with arbitrary napi_callback_info.
template<typename ReturnType, typename... ArgTypes, PropertyMethodType type>
inline std::function<NodeCallbackSig> WrapPropertyMethod(
    PropertyMethodHolder<ReturnType(ArgTypes...), type> holder) {
  static_assert(type != PropertyMethodType::Setter,
                "Setter should not return value");
  return [holder = std::move(holder)](napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(&args, holder.flags);
    if (!invoker.IsOK())
      return static_cast<napi_value>(nullptr);
    return ToNode(env, invoker.DispatchToCallback(holder.callback));
  };
}

template<typename... ArgTypes, PropertyMethodType type>
inline std::function<NodeCallbackSig> WrapPropertyMethod(
    PropertyMethodHolder<void(ArgTypes...), type> holder) {
  static_assert(type != PropertyMethodType::Getter,
                "Getter should return value");
  return [holder = std::move(holder)](napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(&args, holder.flags);
    if (invoker.IsOK())
      invoker.DispatchToCallback(holder.callback);
    return nullptr;
  };
}

// Helper to create PropertyMethodHolder.
template<typename T, PropertyMethodType type>
inline auto CreatePropertyMethodHolder(T func) {
  return PropertyMethodHolder<typename CallbackHolderFactory<T>::RunType, type>(
      CallbackHolderFactory<T>::Create(std::move(func)));
}

}  // namespace internal

}  // namespace nb

#endif  // SRC_PROPERTY_INTERNAL_H_
