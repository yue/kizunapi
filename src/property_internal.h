// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PROPERTY_INTERNAL_H_
#define SRC_PROPERTY_INTERNAL_H_

#include "src/callback_internal.h"

namespace nb {

namespace internal {

// The type of the callback.
enum class CallbackType {
  Method,
  Getter,
  Setter,
};

// Extends CallbackHolder with information about callback type.
template<typename Sig, CallbackType type>
struct PropertyMethodHolder : public CallbackHolder<Sig> {
  explicit PropertyMethodHolder(CallbackHolder<Sig>&& holder)
      : CallbackHolder<Sig>(std::move(holder)) {}
};

// Extends CallbackHolderFactory to support member object pointers.
template<typename T, CallbackType type, typename Enable = void>
struct PropertyMethodHolderFactory {
  using RunType = typename CallbackHolderFactory<T>::RunType;
  using HolderT = PropertyMethodHolder<RunType, type>;
  static inline HolderT Create(T func) {
    return HolderT(CallbackHolderFactory<T>::Create(std::move(func)));
  }
};

template<typename T>
struct PropertyMethodHolderFactory<T, CallbackType::Getter,
                                   typename std::enable_if<
                                       std::is_member_object_pointer<
                                           T>::value>::type> {
  using ClassType = typename ExtractMemberPointer<T>::ClassType;
  using MemberType = typename ExtractMemberPointer<T>::MemberType;
  using RunType = MemberType(ClassType*);
  using HolderT = PropertyMethodHolder<RunType, CallbackType::Getter>;
  static inline HolderT Create(T member_ptr) {
    std::function<RunType> func = [member_ptr](ClassType* p) {
      return p->*member_ptr;
    };
    return HolderT(CallbackHolderFactory<decltype(func)>::Create(
        std::move(func), HolderIsFirstArgument));
  }
};

template<typename T>
struct PropertyMethodHolderFactory<T, CallbackType::Setter,
                                   typename std::enable_if<
                                       std::is_member_object_pointer<
                                           T>::value>::type> {
  using ClassType = typename ExtractMemberPointer<T>::ClassType;
  using MemberType = typename ExtractMemberPointer<T>::MemberType;
  using RunType = void(ClassType*, MemberType);
  using HolderT = PropertyMethodHolder<RunType, CallbackType::Setter>;
  static inline HolderT Create(T member_ptr) {
    std::function<RunType> func = [member_ptr](ClassType* p, MemberType m) {
      p->*member_ptr = std::move(m);
    };
    return HolderT(CallbackHolderFactory<decltype(func)>::Create(
        std::move(func), HolderIsFirstArgument));
  }
};

}  // namespace internal

}  // namespace nb

#endif  // SRC_PROPERTY_INTERNAL_H_
