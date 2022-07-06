// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <nbind.h>

namespace {

void ReturnVoid() {
}

int AddOne(int input) {
  return input + 1;
}

class TestClass {
 public:
  explicit TestClass(int data) : data(data) {
  }

  void Method(int add) {
    data += add;
  }

  int Data() {
    return data;
  }

 private:
  int data;
};

std::string Append64(std::function<std::string()> callback) {
  return callback() + "64";
}

}  // namespace

namespace nb {

template<>
struct Type<TestClass*> {
  static constexpr const char* name = "TestClass";
  static inline napi_status ToNode(napi_env env,
                                   TestClass* value,
                                   napi_value* result) {
    return napi_create_external(env, value, nullptr, nullptr, result);
  }
  static napi_status FromNode(napi_env env,
                              napi_value value,
                              TestClass** out) {
    return napi_get_value_external(env, value, reinterpret_cast<void**>(out));
  }
};

}  // namespace nb

void run_callback_tests(napi_env env, napi_value binding) {
  nb::Set(env, binding, "returnVoid", &ReturnVoid);
  nb::Set(env, binding, "addOne", &AddOne);

  TestClass* object = new TestClass(8963);
  nb::Set(env, binding, "object", object);
  nb::Set(env, binding, "method", &TestClass::Method);
  nb::Set(env, binding, "data", &TestClass::Data);

  nb::Set(env, binding, "append64", &Append64);
}
