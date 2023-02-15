// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <kizunapi.h>

#if defined(WIN32)
#include <windows.h>
#endif

namespace {

void AddFinalizer(napi_env env, napi_value object,
                  std::function<void()> callback) {
  auto holder = std::make_unique<std::function<void()>>(std::move(callback));
  napi_status s = napi_add_finalizer(env, object, holder.get(),
                                     [](napi_env, void* ptr, void*) {
    auto* func = static_cast<std::function<void()>*>(ptr);
    (*func)();
    delete func;
  }, nullptr, nullptr);
  if (s != napi_ok)
    return;
  holder.release();
}

napi_value GetAttachedTable(napi_env env, napi_value value) {
  return ki::AttachedTable(env, value).Value();
}

}  // namespace

#define TEST(Name) \
  {  \
    void run_##Name##_tests(napi_env env, napi_value exports); \
    napi_value binding = ki::CreateObject(env); \
    ki::Set(env, exports, #Name, binding); \
    run_##Name##_tests(env, binding); \
  }

napi_value Init(napi_env env, napi_value exports) {
#if defined(WIN32)
  SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);
#endif
  ki::Set(env, exports,
          "addFinalizer", &AddFinalizer,
          "getAttachedTable", &GetAttachedTable);
  TEST(callback);
  TEST(persistent);
  TEST(property);
  TEST(prototype);
  TEST(types);
  TEST(wrap_method);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
