// Copyright 2022 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef SRC_ARGUMENTS_H_
#define SRC_ARGUMENTS_H_

#include <sstream>

#include "src/dict.h"

namespace nb {

inline std::string NodeTypeToString(napi_env env, napi_value value) {
  if (!value)
    return "<empty handle>";
  napi_valuetype type;
  napi_status s = napi_typeof(env, value, &type);
  if (s != napi_ok)
    return "<unkown>";
  if (type == napi_undefined)
    return "undefined";
  if (type == napi_null)
    return "null";
  if (type == napi_boolean)
    return "Boolean";
  if (type == napi_number)
    return "Number";
  if (type == napi_string)
    return "String";
  if (type == napi_symbol)
    return "Symbol";
  if (type == napi_function)
    return "Function";
  if (type == napi_external)
    return "External";
  if (type == napi_bigint)
    return "BigInt";
  assert(type == napi_object);
  napi_value constructor;
  std::string name;
  if (Get(env, value, "constructor", &constructor) &&
      Get(env, constructor, "name", &name)) {
    return name;
  }
  return "Object";
}

// Arguments is a wrapper around napi_callback_info that integrates with Type<T>
// to make it easier to marshall arguments and return values between V8 and C++.
class Arguments {
 public:
  Arguments(napi_env env, napi_callback_info info) : env_(env) {
    napi_status s = napi_get_cb_info(env, info, &argc_, NULL, NULL, NULL);
    assert(s == napi_ok);
    argv_.resize(argc_);
    s = napi_get_cb_info(env, info, &argc_,
                         argv_.empty() ? nullptr : &argv_.front(),
                         &this_, &data_);
    assert(s == napi_ok);
  }

  ~Arguments() = default;

  template<typename T>
  bool GetNext(T* out) {
    if (next_ >= Length()) {
      insufficient_arguments_ = true;
      return false;
    }
    return FromNode(env_, argv_[next_++], out);
  }

  template<typename T>
  bool GetThis(T* out) {
    return FromNode(env_, this_, out);
  }

  void ThrowError(const char* target_type_name) const {
    if (insufficient_arguments_) {
      napi_throw_type_error(env_, nullptr, "Insufficient number of arguments.");
      return;
    }

    if (next_ == 0) {
      std::ostringstream ss;
      ss << "Error converting this from " << NodeTypeToString(env_, this_)
         << " to " << target_type_name << ".";
      napi_throw_type_error(env_, nullptr, ss.str().c_str());
      return;
    }

    std::ostringstream ss;
    ss << "Error processing argument at index " << next_ - 1 << ", "
       << "conversion failure from " << NodeTypeToString(env_, argv_[next_ -1])
       << " to " << target_type_name << ".";
    napi_throw_type_error(env_, nullptr, ss.str().c_str());
  }

  size_t Length() const { return argc_; }
  napi_env Env() const { return env_; }
  void* Data() const { return data_; }

 private:
  napi_env env_;

  size_t argc_ = 0;
  std::vector<napi_value> argv_;
  napi_value this_ = nullptr;
  void* data_ = nullptr;

  size_t next_ = 0;
  bool insufficient_arguments_ = false;
};

}  // namespace nb

#endif  // SRC_ARGUMENTS_H_
