// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_INSTANCE_DATA_H_
#define SRC_INSTANCE_DATA_H_

#include <map>
#include <utility>

#include "src/map.h"

namespace ki {

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
  void AddWeakRef(void* key, napi_value value) {
    auto it = weak_refs_.find(key);
    if (it != weak_refs_.end()) {
      it->second.first++;
      it->second.second = Persistent(env_, value, 0);
    } else {
      weak_refs_.emplace(key, std::make_pair(1, Persistent(env_, value, 0)));
    }
  }

  bool GetWeakRef(void* key, napi_value* out) const {
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

  void DeleteWeakRef(void* key) {
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
  std::map<void*, std::pair<uint32_t, Persistent>> weak_refs_;

  const int tag_ = 0x8964;
};

}  // namespace ki

#endif  // SRC_INSTANCE_DATA_H_
