// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <nbind.h>

namespace {

int Function() {
  return 8964;
}

int number = 19890604;

int Getter() {
  return number;
}

void Setter(int n) {
  number = n + 1;
}

class SimpleMember {
 public:
  int data = 89;
};

}  // namespace

namespace nb {

template<>
struct Type<SimpleMember> {
  static constexpr const char* name = "SimpleMember";
  static void* Wrap(SimpleMember* ptr) {
    return ptr;
  }
  static void Finalize(void* ptr) {
    delete static_cast<SimpleMember*>(ptr);
  }
  static PropertyList Prototype() {
    return {
      nb::Property("getter", nb::Getter(&SimpleMember::data)),
      nb::Property("setter", nb::Setter(&SimpleMember::data)),
      nb::Property("data", &SimpleMember::data),
    };
  }
};

}  // namespace nb

void run_property_tests(napi_env env, napi_value binding) {
  nb::DefineProperties(
      env, binding,
      nb::Method("method1", &Function),
      nb::Property("method2", &Function),
      nb::Property("method3", nb::Method(&Function)),
      nb::Property("value", nb::ToNode(env, "value")),
      nb::Property("number", nb::Getter(&Getter), nb::Setter(&Setter)));
  nb::Set(env, binding, "member", new SimpleMember);
}
