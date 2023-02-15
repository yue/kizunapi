// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <kizunapi.h>

namespace {

class PersistentMap {
 public:
  void Set(napi_env env, int key, napi_value value) {
    handles_.emplace(key, ki::Persistent(env, value));
  }

  napi_value Get(napi_env env, int key) const {
    auto it = handles_.find(key);
    if (it == handles_.end())
      return ki::ToNode(env, nullptr);
    return it->second.Value();
  }

  void MakeWeak(int key) {
    auto it = handles_.find(key);
    if (it != handles_.end())
      it->second.MakeWeak();
  }

 private:
  std::map<int, ki::Persistent> handles_;
};

}  // namespace

namespace ki {

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

}  // namespace ki

void run_persistent_tests(napi_env env, napi_value binding) {
  ki::Set(env, binding,
          "PersistentMap", ki::Class<PersistentMap>());
}
