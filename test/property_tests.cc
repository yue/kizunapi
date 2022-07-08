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

struct SimpleMember {
  int data = 89;
};

struct HasObjectMember {
  SimpleMember* member = new SimpleMember;
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
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    DefineProperties(env, prototype,
                     Property("getter", Getter(&SimpleMember::data)),
                     Property("setter", Setter(&SimpleMember::data)),
                     Property("data", &SimpleMember::data));
  }
};

template<>
struct Type<HasObjectMember> {
  static constexpr const char* name = "HasObjectMember";
  static HasObjectMember* Constructor() {
    return new HasObjectMember;
  }
  static void Destructor(HasObjectMember* ptr) {
    delete ptr;
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    DefineProperties(env, prototype,
                     Property("member", &HasObjectMember::member));
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
  nb::Set(env, binding,
          "member", new SimpleMember,
          "HasObjectMember", nb::Constructor<HasObjectMember>());
}
