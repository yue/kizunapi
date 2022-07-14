// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <nbind.h>

namespace {

class PersistentMap {
 public:
  void Set(napi_env env, int key, napi_value value) {
    handles_.emplace(key, nb::Persistent(env, value));
  }

  napi_value Get(napi_env env, int key) const {
    auto it = handles_.find(key);
    if (it == handles_.end())
      return nb::ToNode(env, nullptr);
    return it->second.Get();
  }

  void MakeWeak(int key) {
    auto it = handles_.find(key);
    if (it != handles_.end())
      it->second.MakeWeak();
  }

 private:
  std::map<int, nb::Persistent> handles_;
};

}  // namespace

namespace nb {

template<>
struct Type<PersistentMap> {
  static constexpr const char* name = "PersistentMap";
  static PersistentMap* Constructor() {
    return new PersistentMap;
  }
  static void Destructor(PersistentMap* ptr) {
    delete ptr;
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype,
        "set", &PersistentMap::Set,
        "get", &PersistentMap::Get,
        "makeWeak", &PersistentMap::MakeWeak);
  }
};

}  // namespace nb

void run_persistent_tests(napi_env env, napi_value binding) {
  nb::Set(env, binding,
          "PersistentMap", nb::Class<PersistentMap>());
}
