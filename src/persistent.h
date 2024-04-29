// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_PERSISTENT_H_
#define SRC_PERSISTENT_H_

#include <assert.h>
#include <node_api.h>

namespace ki {

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
        bool success = WeakRefFromRef(env_, other.ref_, &ref_);
        assert(other.is_weak_ || success);
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
    bool success = WeakRefFromRef(env_, ref_, &ref_);
    assert(success);
  }

  template<typename T>
  T ToLocal() {
    return T(env_, Value());
  }

  napi_env Env() const {
    return env_;
  }

  napi_value Value() const {
    napi_value result = nullptr;
    napi_get_reference_value(env_, ref_, &result);
    return result;
  }

  bool IsEmpty() const {
    return !ref_;
  }

  napi_ref Id() const {
    return ref_;
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

  static napi_status WeakRefFromRef(napi_env env, napi_ref ref,
                                    napi_ref* new_ref) {
    napi_value value = nullptr;
    napi_status s = napi_get_reference_value(env, ref, &value);
    if (s != napi_ok)
      return s;
    return napi_create_reference(env, value, 0, new_ref);
  }

  napi_env env_ = nullptr;
  napi_ref ref_ = nullptr;
  bool is_weak_ = false;
};

template<>
struct Type<Persistent> {
  static constexpr const char* name = "Value";
  static napi_status ToNode(napi_env env,
                            const ki::Persistent& handle,
                            napi_value* result) {
    return napi_get_reference_value(env, handle.Id(), result);
  }
  static inline std::optional<Persistent> FromNode(napi_env env,
                                                   napi_value value) {
    return Persistent(env, value);
  }
};

}  // namespace ki

#endif  // SRC_PERSISTENT_H_
