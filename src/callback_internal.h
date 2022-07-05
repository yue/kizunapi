// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_CALLBACK_INTERNAL_H_
#define SRC_CALLBACK_INTERNAL_H_

#include <functional>

#include "src/arguments.h"
#include "src/persistent.h"

namespace nb {

enum CreateFunctionTemplateFlags {
  HolderIsFirstArgument = 1 << 0,
};

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

// CallbackHolder is used to pass a std::function from CreateFunctionTemplate
// through DispatchToCallback, where it is invoked.
template<typename Sig>
struct CallbackHolder {
  std::function<Sig> callback;
  int flags = 0;
};

template<typename T>
bool GetNextArgument(Arguments* args, int create_flags, bool is_first,
                     T* result) {
  if (is_first && (create_flags & HolderIsFirstArgument) != 0) {
    return args->GetThis(result);
  } else {
    return args->GetNext(result);
  }
}

// For advanced use cases, we allow callers to request the unparsed Arguments
// object and poke around in it directly.
inline bool GetNextArgument(Arguments* args, int create_flags, bool is_first,
                            Arguments* result) {
  *result = *args;
  return true;
}
inline bool GetNextArgument(Arguments* args, int create_flags, bool is_first,
                            Arguments** result) {
  *result = args;
  return true;
}

// It's common for clients to just need the env, so we make that easy.
inline bool GetNextArgument(Arguments* args, int create_flags, bool is_first,
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

  ArgumentHolder(Arguments* args, int create_flags)
      : ok(GetNextArgument(args, create_flags, index == 0, &value)) {
    if (!ok)
      args->ThrowError(Type<ArgLocalType>::name);
  }
};

// Create scope for executing callbacks.
class CallbackScope {
 public:
  explicit CallbackScope(napi_env env) : env_(env) {
    napi_status s = napi_async_init(env,
                                    CreateObject(env),
                                    ToNode(env, nullptr),
                                    &context_);
    assert(s == napi_ok);
    s = napi_open_callback_scope(env, nullptr, context_, &scope_);
    assert(s == napi_ok);
  }

  ~CallbackScope() {
    napi_status s = napi_close_callback_scope(env_, scope_);
    assert(s == napi_ok);
    s = napi_async_destroy(env_, context_);
    assert(s == napi_ok);
  }

  CallbackScope& operator=(const CallbackScope&) = delete;
  CallbackScope(const CallbackScope&) = delete;

  napi_async_context context() { return context_; }

 private:
  napi_env env_;
  napi_async_context context_;
  napi_callback_scope scope_;
};

// Create scope for handle.
class HandleScope {
 public:
  explicit HandleScope(napi_env env) : env_(env) {
    napi_status s = napi_open_handle_scope(env, &scope_);
    assert(s == napi_ok);
  }

  ~HandleScope() {
    napi_status s = napi_close_handle_scope(env_, scope_);
    assert(s == napi_ok);
  }

  HandleScope& operator=(const HandleScope&) = delete;
  HandleScope(const HandleScope&) = delete;

 private:
  napi_env env_;
  napi_handle_scope scope_;
};


// Create CallbackHolder for function pointers.
template<typename T, typename Enable = void>
struct CallbackHolderFactory {};

template<typename Sig>
struct CallbackHolderFactory<std::function<Sig>> {
  using RunType = Sig;
  using HolderT = CallbackHolder<Sig>;
  static inline CallbackHolder<Sig> Create(std::function<Sig> func) {
    return CallbackHolder<Sig>(std::move(func));
  }
};

template<typename T>
struct CallbackHolderFactory<T, typename std::enable_if<
                                    is_function_pointer<T>::value>::type> {
  using RunType = typename FunctorTraits<T>::RunType;
  using HolderT = CallbackHolder<RunType>;
  static inline HolderT Create(T func) {
    return HolderT{std::function<RunType>(func)};
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
  Invoker(Arguments* args, int create_flags = 0)
      : ArgumentHolder<indices, ArgTypes>(args, create_flags)..., args_(args) {
    // GCC thinks that create_flags is going unused, even though the
    // expansion above clearly makes use of it. Per jyasskin@, casting
    // to void is the commonly accepted way to convince the compiler
    // that you're actually using a parameter/varible.
    (void)create_flags;
  }

  bool IsOK() {
    return And(ArgumentHolder<indices, ArgTypes>::ok...);
  }

  template<typename ReturnType>
  ReturnType DispatchToCallback(
      const std::function<ReturnType(ArgTypes...)>& callback) {
    CallbackScope scope(args_->Env());
    return callback(std::move(ArgumentHolder<indices, ArgTypes>::value)...);
  }

  // In C++, you can declare the function foo(void), but you can't pass a void
  // expression to foo. As a result, we must specialize the case of Callbacks
  // that have the void return type.
  void DispatchToCallback(
      const std::function<void(ArgTypes...)>& callback) {
    CallbackScope scope(args_->Env());
    callback(std::move(ArgumentHolder<indices, ArgTypes>::value)...);
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
struct CFunctionInvoker {};

template<typename ReturnType, typename... ArgTypes>
struct CFunctionInvoker<ReturnType(ArgTypes...)> {
  static ReturnType Invoke(Arguments* args) {
    using HolderT = CallbackHolder<ReturnType(ArgTypes...)>;
    HolderT* holder = static_cast<HolderT*>(args->Data());
    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(args, holder->flags);
    if (!invoker.IsOK())
      return ReturnType();
    return invoker.DispatchToCallback(holder->callback);
  }
};

// Wrap std::function to napi_callback.
template<typename Sig>
struct CFunctionWrapper {
  static napi_value Call(napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    return ToNode(env, CFunctionInvoker<Sig>::Invoke(&args));
  }
};

template<typename... ArgTypes>
struct CFunctionWrapper<void(ArgTypes...)> {
  static napi_value Call(napi_env env, napi_callback_info info) {
    Arguments args(env, info);
    CFunctionInvoker<void(ArgTypes...)>::Invoke(&args);
    return nullptr;
  }
};

// Create a JS Function that executes a provided C++ function or std::function.
// JavaScript arguments are automatically converted via Type<T>, as is
// the return value of the C++ function, if any.
template<typename T>
inline napi_status CreateNodeFunction(napi_env env, T func,
                                      napi_value* result) {
  using Factory = CallbackHolderFactory<T>;
  using RunType = typename Factory::RunType;
  using HolderT = typename Factory::HolderT;
  HolderT holder = Factory::Create(std::move(func));
  return napi_create_function(env, nullptr, 0,
                              &CFunctionWrapper<RunType>::Call,
                              new HolderT{std::move(holder)},
                              result);
}

// Helper to invoke a V8 function with C++ parameters.
template<typename Sig>
struct V8FunctionInvoker {};

template<typename... ArgTypes>
struct V8FunctionInvoker<void(ArgTypes...)> {
  static void Go(napi_env env, Persistent handle, ArgTypes&&... raw) {
    CallbackScope callback_scope(env);
    HandleScope handle_scope(env);
    napi_value func = handle.Get();
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
    assert(s == napi_ok || s == napi_pending_exception);
  }
};

template<typename ReturnType, typename... ArgTypes>
struct V8FunctionInvoker<ReturnType(ArgTypes...)> {
  static ReturnType Go(napi_env env, Persistent handle, ArgTypes&&... raw) {
    CallbackScope callback_scope(env);
    HandleScope handle_scope(env);
    ReturnType ret;
    napi_value func = handle.Get();
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
    assert(s == napi_ok || s == napi_pending_exception);
    if (s == napi_ok)
      FromNode(env, value, &ret);
    return ret;
  }
};

}  // namespace internal

}  // namespace nb

#endif  // SRC_CALLBACK_INTERNAL_H_
