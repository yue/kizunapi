// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include <nbind.h>

#if defined(WIN32)
#include <windows.h>
#endif

#define TEST(Name) \
  {  \
    void run_##Name##_tests(napi_env env, napi_value exports); \
    napi_value binding = nb::CreateObject(env); \
    nb::Set(env, exports, #Name, binding); \
    run_##Name##_tests(env, binding); \
  }

napi_value Init(napi_env env, napi_value exports) {
#if defined(WIN32)
  SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);
#endif
  TEST(types);
  TEST(callback);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
