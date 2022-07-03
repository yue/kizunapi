#include <nbind.h>

#define TEST(Name) \
  void run_##Name##_tests(napi_env env, napi_value exports); \
  napi_value binding = nb::CreateObject(env); \
  nb::Set(env, exports, #Name, binding); \
  run_##Name##_tests(env, binding);

napi_value Init(napi_env env, napi_value exports) {
  TEST(types);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
