// Copyright (c) Microsoft Corporation.
// Copyright (c) The Chromium Authors.
// Licensed under the MIT License.

#ifndef SRC_CALLBACK_INTERNAL_H_
#define SRC_CALLBACK_INTERNAL_H_

#include <functional>

#include "src/arguments.h"
#include "src/napi_util.h"
#include "src/persistent.h"

namespace ki {

enum CallbackConvertionFlags {
  // The |this| object should be passed as first argument.
  HolderIsFirstArgument = 1 << 0,
  // Function passed in the arguments only hold weak reference.
  FunctionArgumentIsWeakRef = 1 << 1,
};

namespace internal {

// Deduce the proper type for callback parameters.
template<typename T>
struct CallbackParamTraits {
  using LocalType = std::decay_t<T>;
};
template<typename T>
struct CallbackParamTraits<const T*> {
  using LocalType = T*;
};
template<>
struct CallbackParamTraits<const char*> {
  using LocalType = const char*;
};
template<>
struct CallbackParamTraits<const char*&> {
  using LocalType = const char*;
};

// The supported function types for conversion.
template<typename T, typename Enable = void>
struct IsFunctionConversionSupported
    : std::integral_constant<
          bool,
          is_function_pointer<T>::value ||
          std::is_function<T>::value ||
          std::is_member_function_pointer<T>::value> {};

// Helper to read C++ args from JS args.
template<typename T>
struct ArgConverter {
  static inline std::optional<T> GetNext(
      Arguments* args, int flags, bool is_first) {
    if (is_first && (flags & HolderIsFirstArgument) != 0) {
      return args->GetThis<T>();
    } else {
      return args->GetNext<T>();
    }
  }
};

// Allow optional arguments at the end.
template<typename T>
struct ArgConverter<std::optional<T>> {
  static inline std::optional<std::optional<T>> GetNext(
      Arguments* args, int flags, bool is_first) {
    std::optional<T> result = ArgConverter<T>::GetNext(args, flags, is_first);
    if (!result && !args->NoMoreArgs())  // error if type mis-match
      return std::nullopt;
    return result;
  }
};

// Allow optional variant at the end if it contains monostate.
template<typename... ArgTypes>
struct ArgConverter<std::variant<std::monostate, ArgTypes...>> {
  using V = std::variant<std::monostate, ArgTypes...>;
  static inline std::optional<V> GetNext(
      Arguments* args, int flags, bool is_first) {
    std::optional<V> result = args->GetNext<V>();
    if (result)  // success conversion
      return result;
    if (args->NoMoreArgs())  // arg is omitted
      return std::monostate();
    return std::nullopt;
  }
};

// Implementation of the FunctionArgumentIsWeakRef flag.
template<typename Sig>
struct ArgConverter<std::function<Sig>> {
  static inline std::optional<std::function<Sig>> GetNext(
      Arguments* args, int flags, bool is_first) {
    if ((flags & FunctionArgumentIsWeakRef) != 0)
      return args->GetNextWeakFunction<Sig>();
    else
      return args->GetNext<std::function<Sig>>();
  }
};

// For advanced use cases, we allow callers to request the unparsed Arguments
// object and poke around in it directly.
template<>
struct ArgConverter<Arguments> {
  static inline std::optional<Arguments> GetNext(
      Arguments* args, int flags, bool is_first) {
    return *args;
  }
};
template<>
struct ArgConverter<Arguments*> {
  static inline std::optional<Arguments*> GetNext(
      Arguments* args, int flags, bool is_first) {
    return args;
  }
};

// It's common for clients to just need the env, so we make that easy.
template<>
struct ArgConverter<napi_env> {
  static inline std::optional<napi_env> GetNext(
      Arguments* args, int flags, bool is_first) {
    return args->Env();
  }
};

// Class template for extracting and storing single argument for callback
// at position |index|.
template<size_t index, typename ArgType>
struct ArgumentHolder {
  using LocalType = typename CallbackParamTraits<ArgType>::LocalType;

  std::optional<LocalType> value;

  ArgumentHolder(Arguments* args, int flags)
      : value(ArgConverter<LocalType>::GetNext(args, flags, index == 0)) {
    if (!value)
      args->ThrowError(Type<LocalType>::name);
  }
};

// CallbackHolder holds information about a std::function.
template<typename Sig>
struct CallbackHolder {
  explicit CallbackHolder(std::function<Sig> callback, int flags = 0)
      : callback(std::move(callback)), flags(flags) {}
  CallbackHolder(const CallbackHolder<Sig>&) = default;
  CallbackHolder(CallbackHolder<Sig>&&) = default;

  std::function<Sig> callback;
  int flags;
};

// Create CallbackHolder for various function types.
template<typename T, typename Enable = void>
struct CallbackHolderFactory {};

template<typename Sig>
struct CallbackHolderFactory<std::function<Sig>> {
  using RunType = Sig;
  using HolderT = CallbackHolder<Sig>;
  static inline HolderT Create(std::function<Sig> func, int flags = 0) {
    return HolderT(std::move(func), flags);
  }
};

template<typename T>
struct CallbackHolderFactory<T, typename std::enable_if<
                                    is_function_pointer<T>::value>::type> {
  using RunType = typename FunctorTraits<T>::RunType;
  using HolderT = CallbackHolder<RunType>;
  static inline HolderT Create(T func, int flags = 0) {
    return HolderT{std::function<RunType>(func), flags};
  }
};

template<typename T>
struct CallbackHolderFactory<T, typename std::enable_if<
                                    std::is_member_function_pointer<
                                        T>::value>::type> {
  using RunType = typename FunctorTraits<T>::RunType;
  using HolderT = CallbackHolder<RunType>;
  static inline HolderT Create(T func, int flags = 0) {
    return HolderT{std::function<RunType>(func), HolderIsFirstArgument | flags};
  }
};

// Class template for converting arguments from JavaScript to C++ and running
// the callback with them.
template<typename IndicesType, typename... ArgTypes>
class Invoker {};

template<size_t... indices, typename... ArgTypes>
class Invoker<IndicesHolder<indices...>, ArgTypes...>
    : public ArgumentHolder<indices, ArgTypes>... {
 public:
  // Invoker<> inherits from ArgumentHolder<> for each argument.
  // C++ has always been strict about the class initialization order,
  // so it is guaranteed ArgumentHolders will be initialized (and thus, will
  // extract arguments from Arguments) in the right order.
  Invoker(Arguments* args, int flags)
      : ArgumentHolder<indices, ArgTypes>(args, flags)..., args_(args) {}

  bool IsOK() {
    return And(ArgumentHolder<indices, ArgTypes>::value.has_value()...);
  }

  template<typename ReturnType>
  ReturnType DispatchToCallback(
      const std::function<ReturnType(ArgTypes...)>& callback) {
    return callback(std::move(*ArgumentHolder<indices, ArgTypes>::value)...);
  }

 private:
  static bool And() { return true; }
  template<typename... T>
  static bool And(bool arg1, T... args) {
    return arg1 && And(args...);
  }

  Arguments* args_;
};

// Converts all the JavaScript arguments to C++ types and invokes the
// std::function.
template<typename Sig>
struct CallbackInvoker {};

template<typename ReturnType, typename... ArgTypes>
struct CallbackInvoker<ReturnType(ArgTypes...)> {
  using HolderT = CallbackHolder<ReturnType(ArgTypes...)>;
  using ReturnLocalType = std::optional<std::decay_t<ReturnType>>;
  static inline ReturnLocalType Invoke(napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    return Invoke(&args);
  }
  static inline ReturnLocalType Invoke(napi_env env, napi_callback_info info,
                                       const HolderT* holder) {
    Arguments args(env, info);
    return Invoke(&args, holder);
  }
  static inline ReturnLocalType Invoke(Arguments* args) {
    return Invoke(args, static_cast<HolderT*>(args->Data()));
  }
  static inline ReturnLocalType Invoke(Arguments* args,
                                       const HolderT* holder,
                                       bool* success = nullptr) {
    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(args, holder->flags);
    if (!invoker.IsOK()) {
      if (success)
        *success = false;
      return std::nullopt;
    }
    if (success)
      *success = true;
#if defined(__cpp_exceptions)
    try {
#endif
      return invoker.DispatchToCallback(holder->callback);
#if defined(__cpp_exceptions)
    } catch (const std::exception& e) {
      ThrowError(args->Env(), e.what());
      if (success)
        *success = false;
      return std::nullopt;
    }
#endif
  }
};

template<typename... ArgTypes>
struct CallbackInvoker<void(ArgTypes...)> {
  using HolderT = CallbackHolder<void(ArgTypes...)>;
  static inline void Invoke(napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    Invoke(&args);
  }
  static inline void Invoke(napi_env env, napi_callback_info info,
                            const HolderT* holder) {
    Arguments args(env, info);
    Invoke(&args, holder);
  }
  static inline void Invoke(Arguments* args) {
    Invoke(args, static_cast<HolderT*>(args->Data()));
  }
  static inline void Invoke(Arguments* args,
                            const HolderT* holder,
                            bool* success = nullptr) {
    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(args, holder->flags);
    if (!invoker.IsOK()) {
      if (success)
        *success = false;
      return;
    }
    if (success)
      *success = true;
#if defined(__cpp_exceptions)
    try {
#endif
      invoker.DispatchToCallback(holder->callback);
#if defined(__cpp_exceptions)
    } catch (const std::exception& e) {
      ThrowError(args->Env(), e.what());
      if (success)
        *success = false;
    }
#endif
  }
};

// Convert the return value of callback to napi_value.
template<typename Sig>
struct ReturnToNode {
  static napi_value Invoke(napi_env env, napi_callback_info info) {
    return ToNodeValue(env, CallbackInvoker<Sig>::Invoke(env, info));
  }
  static napi_value InvokeWithHolder(napi_env env, napi_callback_info info,
                                     const CallbackHolder<Sig>* holder) {
    return ToNodeValue(env, CallbackInvoker<Sig>::Invoke(env, info, holder));
  }
  static napi_value InvokeWithHolder(Arguments* args,
                                     const CallbackHolder<Sig>* holder,
                                     bool* success = nullptr) {
    return ToNodeValue(args->Env(),
                  CallbackInvoker<Sig>::Invoke(args, holder, success));
  }
};

template<typename... ArgTypes>
struct ReturnToNode<void(ArgTypes...)> {
  using Sig = void(ArgTypes...);
  static napi_value Invoke(napi_env env, napi_callback_info info) {
    CallbackInvoker<Sig>::Invoke(env, info);
    return nullptr;
  }
  static napi_value InvokeWithHolder(napi_env env, napi_callback_info info,
                                     const CallbackHolder<Sig>* holder) {
    CallbackInvoker<Sig>::Invoke(env, info, holder);
    return nullptr;
  }
  static napi_value InvokeWithHolder(Arguments* args,
                                     const CallbackHolder<Sig>* holder,
                                     bool* success = nullptr) {
    CallbackInvoker<Sig>::Invoke(args, holder, success);
    return nullptr;
  }
};

using NodeCallbackSig = napi_value(napi_env, napi_callback_info);

// Create a std::function<napi_callback> from the passed |holder|, that can be
// executed with arbitrary napi_callback_info.
template<typename Sig>
inline std::function<NodeCallbackSig> CreateNodeCallbackWithHolder(
    CallbackHolder<Sig>&& holder) {
  return [holder = std::move(holder)](napi_env env, napi_callback_info info) {
    return ReturnToNode<Sig>::InvokeWithHolder(env, info, &holder);
  };
}

// Create a JS Function that executes a provided C++ function or std::function.
// JavaScript arguments are automatically converted via Type<T>, as is
// the return value of the C++ function, if any.
template<typename T>
inline napi_status CreateNodeFunction(napi_env env, T func, napi_value* result,
                                      int flags = 0) {
  using Factory = CallbackHolderFactory<T>;
  using RunType = typename Factory::RunType;
  using HolderT = typename Factory::HolderT;
  auto holder = std::make_unique<HolderT>(Factory::Create(std::move(func),
                                                          flags));
  napi_value intermediate;
  napi_status s = napi_create_function(env, nullptr, 0,
                                       &ReturnToNode<RunType>::Invoke,
                                       holder.get(), &intermediate);
  if (s != napi_ok)
    return s;
  s = AddToFinalizer(env, intermediate, std::move(holder));
  if (s != napi_ok)
    return s;
  *result = intermediate;
  return napi_ok;
}

// Helper to invoke a V8 function with C++ parameters.
template<typename Sig>
struct V8FunctionInvoker {};

template<typename... ArgTypes>
struct V8FunctionInvoker<void(ArgTypes...)> {
  static void Go(napi_env env, Persistent* handle, ArgTypes&&... raw) {
    HandleScope handle_scope(env);
    napi_value func = handle->Value();
    if (!func) {
      ThrowError(env, "The function has been garbage collected");
      return;
    }
    std::vector<napi_value> args = {
        ToNode(env, std::forward<ArgTypes>(raw))...
    };
    napi_status s = napi_make_callback(env, nullptr, func, func, args.size(),
                                       args.empty() ? nullptr: &args.front(),
                                       nullptr);
    if (s == napi_pending_exception) {
      napi_value fatal_exception;
      napi_get_and_clear_last_exception(env, &fatal_exception);
      napi_fatal_exception(env, fatal_exception);
    }
  }
};

template<typename ReturnType, typename... ArgTypes>
struct V8FunctionInvoker<ReturnType(ArgTypes...)> {
  static ReturnType Go(napi_env env, Persistent* handle, ArgTypes&&... raw) {
    HandleScope handle_scope(env);
    ReturnType ret{};
    napi_value func = handle->Value();
    if (!func) {
      ThrowError(env, "The function has been garbage collected");
      return ret;
    }
    std::vector<napi_value> args = {
        ToNode(env, std::forward<ArgTypes>(raw))...
    };
    napi_value value;
    napi_status s = napi_make_callback(env, nullptr, func, func, args.size(),
                                       args.empty() ? nullptr: &args.front(),
                                       &value);
    if (s == napi_ok) {
      std::optional<ReturnType> result = FromNodeTo<ReturnType>(env, value);
      if (result)
        ret = std::move(*result);
    }
    if (s == napi_pending_exception) {
      napi_value fatal_exception;
      napi_get_and_clear_last_exception(env, &fatal_exception);
      napi_fatal_exception(env, fatal_exception);
    }
    return ret;
  }
};

}  // namespace internal

}  // namespace ki

#endif  // SRC_CALLBACK_INTERNAL_H_
