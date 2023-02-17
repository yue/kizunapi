// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <kizunapi.h>

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
  std::function<void()> callback;
};

struct HasObjectMember {
  SimpleMember* member = new SimpleMember;
  SimpleMember* strong = new SimpleMember;
};

}  // namespace

namespace ki {

template<>
struct Type<SimpleMember> {
  static constexpr const char* name = "SimpleMember";
  static void Define(napi_env env, napi_value, napi_value prototype) {
    DefineProperties(env, prototype,
                     Property("getter", Getter(&SimpleMember::data)),
                     Property("setter", Setter(&SimpleMember::data)),
                     Property("data", &SimpleMember::data),
                     Property("callback", Setter(&SimpleMember::callback,
                                                 FunctionArgumentIsWeakRef)));
  }
};

template<>
struct TypeBridge<SimpleMember> {
  static SimpleMember* Wrap(SimpleMember* ptr) {
    return ptr;
  }
  static void Finalize(SimpleMember* ptr) {
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

}  // namespace ki

void run_property_tests(napi_env env, napi_value binding) {
  ki::DefineProperties(
      env, binding,
      ki::Property("value", ki::ToNode(env, "value")),
      ki::Property("number", ki::Getter(&Getter), ki::Setter(&Setter)));
  ki::Set(env, binding,
          "member", new SimpleMember,
          "HasObjectMember", ki::Class<HasObjectMember>());
}
