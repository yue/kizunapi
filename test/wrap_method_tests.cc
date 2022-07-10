// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <nbind.h>

namespace {

class View {
 public:
  ~View() {
    if (child_)
      child_->Release();
  }

  void AddChildView(View* child) {
    child_ = child;
    child_->AddRef();
  }

  void AddEventListener(std::function<void()> callback) {
    callback_ = callback;
  }

  void EmitChild() {
    if (child_ && child_->callback_)
      child_->callback_();
  }

  void AddRef() {
    ref_count_++;
  }

  void Release() {
    if (--ref_count_ == 0)
      delete this;
  }

 public:
  int ref_count_ = 1;
  View* child_ = nullptr;
  std::function<void()> callback_;
};

}  // namespace

namespace nb {

template<>
struct Type<View> {
  static constexpr const char* name = "View";
  static View* Constructor() {
    return new View();
  }
  static void Destructor(View* ptr) {
    ptr->Release();
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype,
        "addChildView",
        WrapMethod(&View::AddChildView, [](const Arguments& args) {
          AttachedTable(args).Set("child", args[0]);
        }),
        "addEventListener", &View::AddEventListener,
        "emitChild", &View::EmitChild);
  }
};

}  // namespace nb

void run_wrap_method_tests(napi_env env, napi_value binding) {
  nb::Set(env, binding, "View", nb::Constructor<View>());
}
