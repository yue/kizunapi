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
  using WeakRefKey = std::pair<const char*, void*>;

  // The garbage collection in V8 has 2 phases:
  // 1. Garbage collect the unused object, which makes the object stored in
  //    weak ref become undefined;
  // 2. Call the finalizer of the object, which usually calls DeleteWeakRef
  //    in it to remove the weak ref.
  //
  // So when we are between 1 and 2, the weak ref is alive because the finalizer
  // has not been called, but the object in it has become undefined. If we call
  // AddWeakRef with the same |key|, which usually happens when converting a ptr
  // to JS immediately after its previous JS wrapper gets GCed, we will have 2
  // different weak refs with the same |key|, with a pending finalizer to remove
  // the previous weak ref.
  //
  // In order to make all finalizers happily call DeleteWeakRef without removing
  // the current weak ref which points to the living object, we are doing a ref
  // counted policy here:
  // 1. In AddWeakRef, if there is a weak ref with |key|, add ref count, and
  //    replace the weak ref with the ref to current value.
  // 2. In DeleteWeakRef, only remove the whole |key| when ref count drops to 0.
  template<typename T>
  void AddWeakRef(void* ptr, napi_value value) {
    WeakRefKey key{internal::TopClass<T>::name, ptr};
    auto it = weak_refs_.find(key);
    if (it != weak_refs_.end()) {
      it->second.first++;
      it->second.second = Persistent(env_, value, 0);
    } else {
      weak_refs_.emplace(key, std::make_pair(1, Persistent(env_, value, 0)));
    }
  }

  template<typename T>
  bool GetWeakRef(void* ptr, napi_value* out) const {
    WeakRefKey key{internal::TopClass<T>::name, ptr};
    auto it = weak_refs_.find(key);
    if (it == weak_refs_.end())
      return false;
    napi_value result = it->second.second.Value();
    if (!result) {
      // Betwen GC phase 1 and 2, the weak ref exists but object becomes
      // undefined, we want to create a new object in this case so return false.
      return false;
    }
    *out = result;
    return true;
  }

  template<typename T>
  void DeleteWeakRef(void* ptr) {
    WeakRefKey key{internal::TopClass<T>::name, ptr};
    auto it = weak_refs_.find(key);
    if (it == weak_refs_.end()) {
      assert(false);
      return;
    }
    if (--it->second.first == 0)
      weak_refs_.erase(it);
  }

 private:
  explicit InstanceData(napi_env env)
      : env_(env),
        attached_tables_(env, WeakMap(env)) {}

  napi_env env_;
  Persistent attached_tables_;
  std::map<void*, Persistent> strong_refs_;
  std::map<WeakRefKey, std::pair<uint32_t, Persistent>> weak_refs_;

  const int tag_ = 0x8964;
};

}  // namespace ki

#endif  // SRC_INSTANCE_DATA_H_
