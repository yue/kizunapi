// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_NAPI_UTIL_H_
#define SRC_NAPI_UTIL_H_

#include <memory>

#include "src/dict.h"

namespace ki {

// Free the |ptr| in |object|'s finalizer.
template<typename T>
napi_status AddToFinalizer(napi_env env, napi_value object,
                           std::unique_ptr<T> ptr) {
  napi_status s = napi_add_finalizer(env, object, ptr.get(),
                                     [](napi_env, void* ptr, void*) {
    delete static_cast<T*>(ptr);
  }, nullptr, nullptr);
  if (s != napi_ok)
    return s;
  ptr.release();
  return napi_ok;
}

// Helper to invoke the |method| of |object| with |args|.
template<typename Name, typename... ArgTypes>
napi_value CallMethod(napi_env env, napi_value object, Name&& method,
                      ArgTypes... args) {
  napi_value func;
  if (!ki::Get(env, object, std::forward<Name>(method), &func))
    return nullptr;
  napi_value argv[] = {ToNode(env, args)...};
  napi_value ret = nullptr;
  napi_call_function(env, object, func, sizeof...(args),
                     sizeof...(args) == 0 ? nullptr : argv, &ret);
  return ret;
}

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

  void* operator new(size_t) = delete;
  void* operator new[] (size_t) = delete;
  void operator delete(void*) = delete;
  void operator delete[] (void*) = delete;

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

  void* operator new(size_t) = delete;
  void* operator new[] (size_t) = delete;
  void operator delete(void*) = delete;
  void operator delete[] (void*) = delete;

 private:
  napi_env env_;
  napi_handle_scope scope_;
};

}  // namespace ki

#endif  // SRC_NAPI_UTIL_H_
