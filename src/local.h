// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_LOCAL_H_
#define SRC_LOCAL_H_

#include <node_api.h>

namespace ki {

class Local {
 public:
  Local() = default;
  Local(napi_env env, napi_value value) : env_(env), value_(value) {}

  operator napi_value() const { return value_; }

  napi_env Env() const { return env_; }
  napi_value Value() const { return value_; }
  bool IsEmpty() const { return value_ != nullptr; }

 private:
  napi_env env_ = nullptr;
  napi_value value_ = nullptr;
};

}  // namespace ki

#endif  // SRC_LOCAL_H_
