// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_MAP_H_
#define SRC_MAP_H_

#include "src/napi_util.h"
#include "src/local.h"

namespace ki {

inline napi_value NewInstanceFromBuiltinType(napi_env env, const char* type) {
  napi_value global = nullptr;
  napi_get_global(env, &global);
  napi_value constructor = nullptr;
  napi_get_named_property(env, global, type, &constructor);
  napi_value instance = nullptr;
  napi_new_instance(env, constructor, 0, nullptr, &instance);
  return instance;
}

class Map : public Local {
 public:
  Map() = default;
  Map(napi_env env, napi_value value) : Local(env, value) {}
  explicit Map(napi_env env) : Map(env, "Map") {}

  template<typename K, typename V>
  void Set(const K& key, const V& value) {
    CallMethod(Env(), Value(), "set", ToNode(Env(), key), ToNode(Env(), value));
  }

  template<typename K, typename V>
  bool Get(const K& key, V* out) const {
    napi_value ret = CallMethod(Env(), Value(), "get", ToNode(Env(), key));
    if (!ret || IsType(Env(), ret, napi_undefined))
      return false;
    return FromNode(Env(), ret, out);
  }

  template<typename K>
  bool Has(const K& key) const {
    return FromNode<bool>(
        Env(),
        CallMethod(Env(), Value(), "has", ToNode(Env(), key))).value_or(false);
  }

  template<typename K>
  void Delete(const K& key) {
    CallMethod(Env(), Value(), "delete", ToNode(Env(), key));
  }

  template<typename K>
  Map GetOrCreateMap(const K& key) {
    napi_value ret;
    if (!Get(key, &ret)) {
      ret = Map(Env()).Value();
      Set(key, ret);
    }
    return Map(Env(), ret);
  }

 protected:
  explicit Map(napi_env env, const char* type)
      : Local(env, NewInstanceFromBuiltinType(env, type)) {}
};

class WeakMap : public Map {
 public:
  explicit WeakMap(napi_env env) : Map(env, "WeakMap") {}
};

}  // namespace ki

#endif  // SRC_MAP_H_
