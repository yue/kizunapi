// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_PERSISTENT_H_
#define SRC_PERSISTENT_H_

#include <assert.h>
#include <node_api.h>

namespace nb {

// RAII managed persistent handle.
class Persistent {
 public:
  Persistent(napi_env env, napi_value value) : env_(env) {
    napi_status s = napi_create_reference(env, value, 1, &ref_);
    assert(s == napi_ok);
  }

  ~Persistent() {
    UnRef();
  }

  Persistent& operator=(const Persistent& other) {
    Copy(other);
    return *this;
  }

  Persistent(const Persistent& other) {
    Copy(other);
  }

  Persistent(Persistent&& other) {
    UnRef();
    env_ = other.env_;
    ref_ = other.ref_;
    other.ref_ = nullptr;
  }

  napi_value Get() const {
    napi_value result = nullptr;
    napi_status s = napi_get_reference_value(env_, ref_, &result);
    assert(s == napi_ok);
    return result;
  }

 private:
  void UnRef() {
    if (!ref_)
      return;
    uint32_t ref_count = 0;
    napi_status s = napi_reference_unref(env_, ref_, &ref_count);
    assert(s == napi_ok);
    if (ref_count == 0) {
      s = napi_delete_reference(env_, ref_);
      assert(s == napi_ok);
    }
  }

  void Copy(const Persistent& other) {
    UnRef();
    env_ = other.env_;
    ref_ = other.ref_;
    napi_status s = napi_reference_ref(env_, ref_, nullptr);
    assert(s == napi_ok);
  }

  napi_env env_;
  napi_ref ref_ = nullptr;
};

}  // namespace nb

#endif  // SRC_PERSISTENT_H_
