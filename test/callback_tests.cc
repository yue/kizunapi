// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <kizunapi.h>

namespace {

void ReturnVoid() {
}

int AddOne(int input) {
  return input + 1;
}

std::string Append64(std::function<std::string()> callback) {
  return callback() + "64";
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

std::function<void()> stored_function;

void StoreWeakFunction(ki::Arguments args) {
  auto result = ki::ConvertWeakFunctionFromNode<void>(args.Env(), args[0]);
  if (result)
    stored_function = std::move(*result);
}

void RunStoredFunction() {
  stored_function();
}

void ClearStoredFunction() {
  stored_function = nullptr;
}

}  // namespace

namespace ki {

template<>
struct Type<TestClass*> {
  static constexpr const char* name = "TestClass";
  static inline napi_status ToNode(napi_env env,
                                   TestClass* value,
                                   napi_value* result) {
    return napi_create_external(env, value, nullptr, nullptr, result);
  }
  static inline std::optional<TestClass*> FromNode(napi_env env,
                                                   napi_value value) {
    TestClass* out;
    if (napi_get_value_external(env, value, reinterpret_cast<void**>(&out))
            == napi_ok)
      return out;
    return std::nullopt;
  }
};

}  // namespace ki

void run_callback_tests(napi_env env, napi_value binding) {
  ki::Set(env, binding, "returnVoid", &ReturnVoid,
                        "addOne", &AddOne,
                        "append64", &Append64,
                        "nullFunction", std::function<void()>());

  TestClass* object = new TestClass(8963);
  ki::Set(env, binding, "object", object,
                        "method", &TestClass::Method,
                        "data", &TestClass::Data);

  ki::Set(env, binding, "storeWeakFunction", &StoreWeakFunction,
                        "runStoredFunction", &RunStoredFunction,
                        "clearStoredFunction", &ClearStoredFunction);
}
