// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_INSTANCE_DATA_H_
#define SRC_INSTANCE_DATA_H_

#include <map>
#include <utility>

#include "src/map.h"

namespace ki {

namespace internal {

// Get the base name of a type.
template<typename T, typename Enable = void>
struct TopClass {
  static constexpr const char* name = Type<std::remove_cv_t<T>>::name;
};

template<typename T>
struct TopClass<T, typename std::enable_if<std::is_class<
                       typename Type<T>::Base>::value>::type> {
  static constexpr const char* name = TopClass<typename Type<T>::Base>::name;
};

}  // namespace internal

class InstanceData {
 public:
  static InstanceData* Get(napi_env env) {
    void* data = nullptr;
    napi_status s = napi_get_instance_data(env, &data);
    assert(s == napi_ok);
    if (!data) {
      data = new InstanceData(env);
      s = napi_set_instance_data(env, data, [](napi_env, void* data, void*) {
        delete static_cast<InstanceData*>(data);
      }, nullptr);
      assert(s == napi_ok);
    }
    InstanceData* ret = static_cast<InstanceData*>(data);
    assert(ret->tag_ == 0x8964);
    return ret;
  }

  // Get or create a object attached to an object.
  Map GetOrCreateAttachedTable(napi_value object) {
    Map lookup = attached_tables_.ToLocal<Map>();
    napi_value value;
    if (lookup.Get(object, &value))
      return Map(env_, value);
    Map table(env_);
    lookup.Set(object, table.Value());
    return table;
  }

  // Add and get persistent handles.
  void Set(void* key, napi_value value) {
    strong_refs_.emplace(key, Persistent(env_, value));
  }

  bool Get(void* key, napi_value* out) const {
    auto it = strong_refs_.find(key);
    if (it == strong_refs_.end())
      return false;
    *out = it->second.Value();
    return true;
  }

  void Delete(void* key) {
    strong_refs_.erase(key);
  }

  // In C++ the address of a class's first member data is equivalent to the
  // address of the class itself, so 1 pointer can actually represent 2
  // different instances. To avoid duplicate key for different instances, we
  // also include typename as part of the key.
  using WrapperKey = std::pair<const char*, void*>;

  // Used to store the results of napi_wrap, it is caller's responsibility to
  // destroy the result.
  template<typename T>
  void AddWrapper(void* ptr, napi_ref ref) {
    WrapperKey key{internal::TopClass<T>::name, ptr};
    wrappers_.emplace(key, Persistent(env_, ref));
  }

  template<typename T>
  bool GetWrapper(void* ptr, napi_value* result) const {
    WrapperKey key{internal::TopClass<T>::name, ptr};
    auto it = wrappers_.find(key);
    if (it == wrappers_.end())
      return false;
    *result = it->second.Value();
    return *result != nullptr;
  }

  template<typename T>
  bool DeleteWrapper(void* ptr) {
    WrapperKey key{internal::TopClass<T>::name, ptr};
    auto it = wrappers_.find(key);
    if (it == wrappers_.end())
      return false;
    wrappers_.erase(key);
    return true;
  }

 private:
  explicit InstanceData(napi_env env)
      : env_(env),
        attached_tables_(env, WeakMap(env)) {}

  ~InstanceData() {
    // Node frees all references on exit whether they belong to user or runtime,
    // so we have to leak them to avoid double free.
    for (auto& [key, handle] : wrappers_) {
      handle.Release();
    }
  }

  napi_env env_;
  Persistent attached_tables_;
  std::map<void*, Persistent> strong_refs_;
  std::map<WrapperKey, Persistent> wrappers_;

  const int tag_ = 0x8964;
};

}  // namespace ki

#endif  // SRC_INSTANCE_DATA_H_
