// Copyright (c) Microsoft Corporation.
// Copyright (c) The Chromium Authors.
// Licensed under the MIT License.

#ifndef SRC_CALLBACK_INTERNAL_H_
#define SRC_CALLBACK_INTERNAL_H_

#include <v8-version.h>

#include <functional>

#include "src/arguments.h"
#include "src/napi_util.h"
#include "src/persistent.h"

namespace ki {

namespace internal {

// Deduce the proper type for callback parameters.
template<typename T>
struct CallbackParamTraits {
  using LocalType = typename std::decay<T>::type;
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

// Flag to indicate the |this| object should be passed as first argument.
enum CreateFunctionTemplateFlags {
  HolderIsFirstArgument = 1 << 0,
};

// Helper to read C++ args from JS args.
template<typename T>
bool GetNextArgument(Arguments* args, int flags, bool is_first, T* result) {
  if (is_first && (flags & HolderIsFirstArgument) != 0) {
    return args->GetThis(result);
  } else {
    return args->GetNext(result);
  }
}

// For advanced use cases, we allow callers to request the unparsed Arguments
// object and poke around in it directly.
inline bool GetNextArgument(Arguments* args, int flags, bool is_first,
                            Arguments* result) {
  *result = *args;
  return true;
}
inline bool GetNextArgument(Arguments* args, int flags, bool is_first,
                            Arguments** result) {
  *result = args;
  return true;
}

// It's common for clients to just need the env, so we make that easy.
inline bool GetNextArgument(Arguments* args, int flags, bool is_first,
                            napi_env* result) {
  *result = args->Env();
  return true;
}

// Class template for extracting and storing single argument for callback
// at position |index|.
template<size_t index, typename ArgType>
struct ArgumentHolder {
  using ArgLocalType = typename CallbackParamTraits<ArgType>::LocalType;

  ArgLocalType value;
  bool ok;

  ArgumentHolder(Arguments* args, int flags)
      : ok(GetNextArgument(args, flags, index == 0, &value)) {
    if (!ok)
      args->ThrowError(Type<ArgLocalType>::name);
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
  static inline HolderT Create(T func) {
    return HolderT{std::function<RunType>(func), HolderIsFirstArgument};
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
    return And(ArgumentHolder<indices, ArgTypes>::ok...);
  }

  template<typename ReturnType>
  ReturnType DispatchToCallback(
      const std::function<ReturnType(ArgTypes...)>& callback) {
    CallbackScope scope(args_->Env());
    return callback(std::move(ArgumentHolder<indices, ArgTypes>::value)...);
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
  static inline ReturnType Invoke(napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    return Invoke(&args);
  }
  static inline ReturnType Invoke(napi_env env, napi_callback_info info,
                                  const HolderT* holder) {
    Arguments args(env, info);
    return Invoke(&args, holder);
  }
  static inline ReturnType Invoke(Arguments* args) {
    return Invoke(args, static_cast<HolderT*>(args->Data()));
  }
  static inline ReturnType Invoke(Arguments* args,
                                  const HolderT* holder,
                                  bool* success = nullptr) {
    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(args, holder->flags);
    if (!invoker.IsOK()) {
      if (success)
        *success = false;
      return ReturnType();
    }
    if (success)
      *success = true;
    return invoker.DispatchToCallback(holder->callback);
  }
};

// Convert the return value of callback to napi_value.
template<typename Sig>
struct ReturnToNode {
  static napi_value Invoke(napi_env env, napi_callback_info info) {
    return ToNode(env, CallbackInvoker<Sig>::Invoke(env, info));
  }
  static napi_value InvokeWithHolder(napi_env env, napi_callback_info info,
                                     const CallbackHolder<Sig>* holder) {
    return ToNode(env, CallbackInvoker<Sig>::Invoke(env, info, holder));
  }
  static napi_value InvokeWithHolder(Arguments* args,
                                     const CallbackHolder<Sig>* holder,
                                     bool* success = nullptr) {
    return ToNode(args->Env(),
                  CallbackInvoker<Sig>::Invoke(args, holder, success));
  }
};

template<typename... ArgTypes>
struct ReturnToNode<void(ArgTypes...)> {
  using RunType = void(ArgTypes...);
  static napi_value Invoke(napi_env env, napi_callback_info info) {
    CallbackInvoker<RunType>::Invoke(env, info);
    return nullptr;
  }
  static napi_value InvokeWithHolder(napi_env env, napi_callback_info info,
                                     const CallbackHolder<RunType>* holder) {
    CallbackInvoker<RunType>::Invoke(env, info, holder);
    return nullptr;
  }
  static napi_value InvokeWithHolder(Arguments* args,
                                     const CallbackHolder<RunType>* holder,
                                     bool* success) {
    if (success)
      *success = true;
    CallbackInvoker<RunType>::Invoke(args, holder);
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
inline napi_status CreateNodeFunction(napi_env env, T func,
                                      napi_value* result) {
  using Factory = CallbackHolderFactory<T>;
  using RunType = typename Factory::RunType;
  using HolderT = typename Factory::HolderT;
  auto holder = std::make_unique<HolderT>(Factory::Create(std::move(func)));
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
  static void Go(napi_env env, Persistent handle, ArgTypes&&... raw) {
    CallbackScope callback_scope(env);
    HandleScope handle_scope(env);
    napi_value func = handle.Value();
    if (!func) {
      napi_throw_error(env, nullptr, "The function has been garbage collected");
      return;
    }
    std::vector<napi_value> args = {
        ToNode(env, std::forward<ArgTypes>(raw))...
    };
    napi_status s = napi_make_callback(env, callback_scope.context(), func,
                                       func, args.size(),
                                       args.empty() ? nullptr: &args.front(),
                                       nullptr);
#if V8_MAJOR_VERSION > 8
    // Executing callback on exit results in error on Node 14.
    assert(s == napi_ok || s == napi_pending_exception);
#endif
  }
};

template<typename ReturnType, typename... ArgTypes>
struct V8FunctionInvoker<ReturnType(ArgTypes...)> {
  static ReturnType Go(napi_env env, Persistent handle, ArgTypes&&... raw) {
    CallbackScope callback_scope(env);
    HandleScope handle_scope(env);
    ReturnType ret{};
    napi_value func = handle.Value();
    if (!func) {
      napi_throw_error(env, nullptr, "The function has been garbage collected");
      return ret;
    }
    std::vector<napi_value> args = {
        ToNode(env, std::forward<ArgTypes>(raw))...
    };
    napi_value value;
    napi_status s = napi_make_callback(env, callback_scope.context(), func,
                                       func, args.size(),
                                       args.empty() ? nullptr: &args.front(),
                                       &value);
#if V8_MAJOR_VERSION > 8
    // Executing callback on exit results in error on Node 14.
    assert(s == napi_ok || s == napi_pending_exception);
#endif
    if (s == napi_ok)
      FromNode(env, value, &ret);
    return ret;
  }
};

}  // namespace internal

}  // namespace ki

#endif  // SRC_CALLBACK_INTERNAL_H_
