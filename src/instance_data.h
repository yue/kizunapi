// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_INSTANCE_DATA_H_
#define SRC_INSTANCE_DATA_H_

#include <map>
#include <memory>
#include <tuple>

#include "src/persistent.h"

namespace nb {

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

  Persistent& Set(void* key, napi_value value) {
    return handles_.emplace(key, Persistent(env_, value)).first->second;
  }

  bool Get(void* key, napi_value* out) const {
    auto it = handles_.find(key);
    if (it == handles_.end())
      return false;
    *out = it->second.Get();
    return true;
  }

  void Remove(void* key) {
    handles_.erase(key);
  }

 private:
  explicit InstanceData(napi_env env) : env_(env) {}

  napi_env env_;
  std::map<void*, Persistent> handles_;

  const int tag_ = 0x8964;
};

}  // namespace nb

#endif  // SRC_INSTANCE_DATA_H_
