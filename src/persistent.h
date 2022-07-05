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
    Destroy();
  }

  Persistent& operator=(const Persistent& other) {
    if (this != &other)
      Copy(other);
    return *this;
  }

  Persistent(const Persistent& other) {
    Copy(other);
  }

  Persistent(Persistent&& other) {
    Destroy();
    env_ = other.env_;
    ref_ = other.ref_;
    is_weak_ = other.is_weak_;
    other.ref_ = nullptr;
  }

  napi_value Get() const {
    napi_value result = nullptr;
    napi_status s = napi_get_reference_value(env_, ref_, &result);
    assert(s == napi_ok);
    return result;
  }

  void MakeWeak() {
    if (is_weak_ || !ref_)
      return;
    is_weak_ = true;
    // If this is the only ref then a unref can make it weak.
    if (Unref() == 0)
      return;
    // Otherwise there are other refs and we must create a new ref.
    ref_ = WeakRefFromRef(env_, ref_);
  }

 private:
  uint32_t Unref() {
    if (!ref_ || is_weak_)
      return 0;
    uint32_t ref_count = 0;
    napi_status s = napi_reference_unref(env_, ref_, &ref_count);
    assert(s == napi_ok);
    return ref_count;
  }

  void Destroy() {
    if (ref_ && Unref() == 0) {
      napi_status s = napi_delete_reference(env_, ref_);
      assert(s == napi_ok);
    }
  }

  void Copy(const Persistent& other) {
    Destroy();
    env_ = other.env_;
    is_weak_ = other.is_weak_;
    if (is_weak_) {
      ref_ = WeakRefFromRef(env_, other.ref_);
    } else {
      ref_ = other.ref_;
      napi_status s = napi_reference_ref(env_, ref_, nullptr);
      assert(s == napi_ok);
    }
  }

  static napi_ref WeakRefFromRef(napi_env env, napi_ref ref) {
    napi_value value = nullptr;
    napi_status s = napi_get_reference_value(env, ref, &value);
    assert(s == napi_ok);
    napi_ref weak = nullptr;
    s = napi_create_reference(env, value, 0, &weak);
    assert(s == napi_ok);
    return weak;
  }

  napi_env env_;
  napi_ref ref_ = nullptr;
  bool is_weak_ = false;
};

}  // namespace nb

#endif  // SRC_PERSISTENT_H_
