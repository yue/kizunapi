// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PERSISTENT_H_
#define SRC_PERSISTENT_H_

#include <assert.h>
#include <node_api.h>

namespace nb {

// RAII managed persistent handle.
class Persistent {
 public:
  Persistent(napi_env env, napi_value value, uint32_t ref_count = 1)
      : env_(env), is_weak_(ref_count == 0) {
    napi_status s = napi_create_reference(env, value, ref_count, &ref_);
    assert(s == napi_ok);
  }

  Persistent() {}

  Persistent(const Persistent& other) {
    *this = other;
  }

  Persistent(Persistent&& other) {
    *this = std::move(other);
  }

  ~Persistent() {
    Destroy();
  }

  Persistent& operator=(const Persistent& other) {
    if (this != &other) {
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
    return *this;
  }

  Persistent& operator=(Persistent&& other) {
    if (this != &other) {
      Destroy();
      env_ = other.env_;
      ref_ = other.ref_;
      is_weak_ = other.is_weak_;
      other.ref_ = nullptr;
    }
    return *this;
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
    // If this is the only ref then a unref can make it weak.
    if (Unref() == 0) {
      is_weak_ = true;
      return;
    }
    // Otherwise there are other refs and we must create a new ref.
    is_weak_ = true;
    ref_ = WeakRefFromRef(env_, ref_);
  }

  bool IsEmpty() const {
    return !ref_;
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

  static napi_ref WeakRefFromRef(napi_env env, napi_ref ref) {
    napi_value value = nullptr;
    napi_status s = napi_get_reference_value(env, ref, &value);
    assert(s == napi_ok);
    napi_ref weak = nullptr;
    s = napi_create_reference(env, value, 0, &weak);
    assert(s == napi_ok);
    return weak;
  }

  napi_env env_ = nullptr;
  napi_ref ref_ = nullptr;
  bool is_weak_ = false;
};

}  // namespace nb

#endif  // SRC_PERSISTENT_H_
