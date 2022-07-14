// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <nbind.h>

namespace {

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
  SimpleMember* strong = new SimpleMember;
};

}  // namespace

namespace nb {

template<>
struct Type<SimpleMember> {
  static constexpr const char* name = "SimpleMember";
  static SimpleMember* Wrap(SimpleMember* ptr) {
    return ptr;
  }
  static void Finalize(SimpleMember* ptr) {
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
                     Property("member", &HasObjectMember::member),
                     Property("strong", &HasObjectMember::strong,
                              Property::CacheMode::GetterAndSetter));
  }
};

}  // namespace nb

void run_property_tests(napi_env env, napi_value binding) {
  nb::DefineProperties(
      env, binding,
      nb::Property("value", nb::ToNode(env, "value")),
      nb::Property("number", nb::Getter(&Getter), nb::Setter(&Setter)));
  nb::Set(env, binding,
          "member", new SimpleMember,
          "HasObjectMember", nb::Class<HasObjectMember>());
}
