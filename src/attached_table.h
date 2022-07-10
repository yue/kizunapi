// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_ATTACHED_TABLE_H_
#define SRC_ATTACHED_TABLE_H_

#include "src/arguments.h"
#include "src/instance_data.h"

namespace nb {

class AttachedTable {
 public:
  AttachedTable() = default;

  explicit AttachedTable(const Arguments& args)
      : AttachedTable(args.Env(), args.GetThis()) {}

  AttachedTable(napi_env env, napi_value object)
      : env_(env),
        table_(InstanceData::Get(env)->GetAttachedTable(object)) {
    assert(IsObject(env, table_));
  }

  template<typename K, typename V>
  bool Get(K&& key, V* out) {
    return nb::Get(env_, table_, std::forward<K>(key), out);
  }

  template<typename K, typename V>
  bool Set(K&& key, V&& value) {
    return nb::Set(env_, table_, std::forward<K>(key), std::forward<V>(value));
  }

  void* operator new(size_t) = delete;
  void* operator new[] (size_t) = delete;
  void operator delete(void*) = delete;
  void operator delete[] (void*) = delete;

 private:
  napi_env env_ = nullptr;
  napi_value table_ = nullptr;
};

}  // namespace nb

#endif  // SRC_ATTACHED_TABLE_H_
