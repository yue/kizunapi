// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_ATTACHED_TABLE_H_
#define SRC_ATTACHED_TABLE_H_

#include "src/arguments.h"
#include "src/instance_data.h"

namespace ki {

class AttachedTable : public Map {
 public:
  AttachedTable() = default;
  AttachedTable(napi_env env, napi_value object)
      : Map(InstanceData::Get(env)->GetOrCreateAttachedTable(object)) {}
  explicit AttachedTable(const Arguments& args)
      : AttachedTable(args.Env(), args.This()) {}

  template<typename K>
  Map GetOrCreateMap(const K& key) {
    napi_value ret;
    if (!Get(key, &ret)) {
      ret = Map(Env()).Value();
      Set(key, ret);
    }
    return Map(Env(), ret);
  }
};

}  // namespace ki

#endif  // SRC_ATTACHED_TABLE_H_
