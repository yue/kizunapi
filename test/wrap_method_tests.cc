// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <kizunapi.h>

namespace {

class View {
 public:
  ~View() {
    if (child_)
      child_->Release();
  }

  void DoNothingWithView(View* v) {
  }

  void AddChildView(View* child) {
    child_ = child;
    child_->AddRef();
  }

  void RemoveChildView(View* child) {
    assert(child == child_);
    child_->Release();
    child_ = nullptr;
  }

  void AddEventListener(std::function<void()> callback) {
    callback_ = callback;
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

namespace ki {

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
        "doNothingWithView", &View::DoNothingWithView,
        "addChildView",
        WrapMethod(&View::AddChildView, [](const Arguments& args) {
          AttachedTable(args).Set(args[0], true);
        }),
        "removeChildView",
        WrapMethod(&View::RemoveChildView, [](const Arguments& args) {
          AttachedTable(args).Delete(args[0]);
        }),
        "addEventListener",
        WrapMethod(&View::AddEventListener, [](const Arguments& args) {
          // Do nothing to verify callback is not strong referenced.
        }));
  }
};

}  // namespace ki

void run_wrap_method_tests(napi_env env, napi_value binding) {
  ki::Set(env, binding, "View", ki::Class<View>());
}
