// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef SRC_TYPES_H_
#define SRC_TYPES_H_

#include <assert.h>
#include <node_api.h>

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "src/template_util.h"

namespace ki {

template<typename T, typename Enable = void>
struct TypeBridge {};

template<typename T, typename Enable = void>
struct Type {};

template<>
struct Type<napi_value> {
  static constexpr const char* name = "Value";
  static inline napi_status ToNode(napi_env env,
                                   napi_value value,
                                   napi_value* result) {
    *result = value;
    return napi_ok;
  }
  static napi_status FromNode(napi_env env, napi_value value, napi_value* out) {
    *out = value;
    return napi_ok;
  }
};

template<>
struct Type<std::nullptr_t> {
  static constexpr const char* name = "Null";
  static inline napi_status ToNode(napi_env env,
                                   std::nullptr_t value,
                                   napi_value* result) {
    return napi_get_null(env, result);
  }
};

template<>
struct Type<int32_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   int32_t value,
                                   napi_value* result) {
    return napi_create_int32(env, value, result);
  }
  static napi_status FromNode(napi_env env, napi_value value, int32_t* out) {
    return napi_get_value_int32(env, value, out);
  }
};

template<>
struct Type<uint32_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   uint32_t value,
                                   napi_value* result) {
    return napi_create_uint32(env, value, result);
  }
  static napi_status FromNode(napi_env env, napi_value value, uint32_t* out) {
    return napi_get_value_uint32(env, value, out);
  }
};

template<>
struct Type<int64_t> {
  static constexpr const char* name = "Integer";
  static inline napi_status ToNode(napi_env env,
                                   int64_t value,
                                   napi_value* result) {
    return napi_create_int64(env, value, result);
  }
  static napi_status FromNode(napi_env env, napi_value value, int64_t* out) {
    return napi_get_value_int64(env, value, out);
  }
};

template<>
struct Type<float> {
  static constexpr const char* name = "Number";
  static inline napi_status ToNode(napi_env env,
                                   float value,
                                   napi_value* result) {
    return napi_create_double(env, value, result);
  }
  static napi_status FromNode(napi_env env, napi_value value, float* out) {
    double intermediate;
    napi_status s = napi_get_value_double(env, value, &intermediate);
    if (s == napi_ok)
      *out = intermediate;
    return s;
  }
};

template<>
struct Type<double> {
  static constexpr const char* name = "Number";
  static inline napi_status ToNode(napi_env env,
                                   double value,
                                   napi_value* result) {
    return napi_create_double(env, value, result);
  }
  static napi_status FromNode(napi_env env, napi_value value, double* out) {
    return napi_get_value_double(env, value, out);
  }
};

template<>
struct Type<bool> {
  static constexpr const char* name = "Boolean";
  static inline napi_status ToNode(napi_env env,
                                   bool value,
                                   napi_value* result) {
    return napi_get_boolean(env, value, result);
  }
  static napi_status FromNode(napi_env env, napi_value value, bool* out) {
    return napi_get_value_bool(env, value, out);
  }
};

template<>
struct Type<std::string> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const std::string& value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value.c_str(), value.length(), result);
  }
  static napi_status FromNode(napi_env env,
                              napi_value value,
                              std::string* out) {
    size_t length;
    napi_status s = napi_get_value_string_utf8(env, value, nullptr, 0, &length);
    if (s != napi_ok)
      return s;
    if (length > 0) {
      out->reserve(length + 1);
      out->resize(length);
      return napi_get_value_string_utf8(env, value, &out->front(),
                                        out->capacity(), nullptr);
    } else {
      out->clear();
      return napi_ok;
    }
  }
};

template<>
struct Type<std::u16string> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const std::u16string& value,
                                   napi_value* result) {
    return napi_create_string_utf16(env, value.c_str(), value.length(), result);
  }
  static napi_status FromNode(napi_env env,
                              napi_value value,
                              std::u16string* out) {
    size_t length;
    napi_status s = napi_get_value_string_utf16(env, value, nullptr, 0,
                                                &length);
    if (s != napi_ok)
      return s;
    if (length > 0) {
      out->reserve(length + 1);
      out->resize(length);
      return napi_get_value_string_utf16(env, value, &out->front(),
                                         out->capacity(), nullptr);
    } else {
      out->clear();
      return napi_ok;
    }
  }
};

template<>
struct Type<const char*> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char* value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, result);
  }
};

template<>
struct Type<char*> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char* value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, result);
  }
};

template<size_t n>
struct Type<char[n]> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char* value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value, n - 1, result);
  }
};

template<>
struct Type<const char16_t*> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char16_t* value,
                                   napi_value* result) {
    return napi_create_string_utf16(env, value, NAPI_AUTO_LENGTH, result);
  }
};

template<>
struct Type<char16_t*> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char16_t* value,
                                   napi_value* result) {
    return napi_create_string_utf16(env, value, NAPI_AUTO_LENGTH, result);
  }
};

template<size_t n>
struct Type<char16_t[n]> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const char16_t* value,
                                   napi_value* result) {
    return napi_create_string_utf16(env, value, n - 1, result);
  }
};

template<>
struct Type<napi_env> {
  static constexpr const char* name = "Environment";
};

// Optimized helper for creating symbols.
template<size_t n>
struct SymbolHolder {
  const char* str;
};

template<size_t n>
inline SymbolHolder<n> Symbol(const char (&value)[n]) {
  SymbolHolder<n> holder;
  holder.str = value;
  return holder;
}

template<size_t n>
struct Type<SymbolHolder<n>> {
  static constexpr const char* name = "Symbol";
  static inline napi_status ToNode(napi_env env,
                                   SymbolHolder<n> value,
                                   napi_value* result) {
    napi_value symbol;
    napi_status s = Type<char[n]>::ToNode(env, value.str, &symbol);
    if (s != napi_ok)
      return s;
    return napi_create_symbol(env, symbol, result);
  }
};

// Some builtin types.
inline napi_value Global(napi_env env) {
  napi_value result;
  napi_status s = napi_get_global(env, &result);
  assert(s == napi_ok);
  return result;
}

inline napi_value Undefined(napi_env env) {
  napi_value result;
  napi_status s = napi_get_undefined(env, &result);
  assert(s == napi_ok);
  return result;
}

inline napi_value Null(napi_env env) {
  napi_value result;
  napi_status s = napi_get_null(env, &result);
  assert(s == napi_ok);
  return result;
}

// Object helpers.
inline napi_value CreateObject(napi_env env) {
  napi_value value = nullptr;
  napi_status s = napi_create_object(env, &value);
  assert(s == napi_ok);
  return value;
}

inline bool IsArray(napi_env env, napi_value value) {
  bool result = false;
  napi_is_array(env, value, &result);
  return result;
}

inline bool IsType(napi_env env, napi_value value, napi_valuetype target) {
  napi_valuetype type;
  napi_status s = napi_typeof(env, value, &type);
  return s == napi_ok && type == target;
}

// Function helpers.
template<typename T>
inline napi_status ConvertToNode(napi_env env, const T& value,
                                 napi_value* result) {
  return Type<T>::ToNode(env, value, result);
}

template<typename T>
inline napi_status ConvertToNode(napi_env env, T&& value,
                                 napi_value* result) {
  return Type<std::decay_t<T>>::ToNode(env, std::forward<T>(value), result);
}

template<typename T>
inline napi_status ConvertFromNode(napi_env env, napi_value value, T* out) {
  return Type<T>::FromNode(env, value, out);
}

// Optimized version for string iterals.
template<size_t n>
inline napi_status ConvertToNode(napi_env env, const char (&value)[n],
                                 napi_value* result) {
  return Type<char[n]>::ToNode(env, value, result);
}

template<size_t n>
inline napi_status ConvertToNode(napi_env env, const char16_t (&value)[n],
                                 napi_value* result) {
  return Type<char16_t[n]>::ToNode(env, value, result);
}

// Convert from In to Out and ignore error.
template<typename Out, typename In>
inline napi_value ConvertIgnoringStatus(napi_env env, In&& value) {
  napi_value result = nullptr;
  napi_status s = Type<Out>::ToNode(env, std::forward<In>(value), &result);
  // Return undefined on error.
  if (s != napi_ok)
    return Undefined(env);
  return result;
}

template<typename T>
inline napi_value ToNode(napi_env env, const T& value) {
  return ConvertIgnoringStatus<T>(env, value);
}

template<typename T>
inline napi_value ToNode(napi_env env, T&& value) {
  return ConvertIgnoringStatus<std::decay_t<T>>(env, std::forward<T>(value));
}

template<typename T>
inline bool FromNode(napi_env env, napi_value value, T* out) {
  return Type<T>::FromNode(env, value, out) == napi_ok;
}

template<size_t n>
inline napi_value ToNode(napi_env env, const char (&value)[n]) {
  return ConvertIgnoringStatus<char[n]>(env, value);
}

template<size_t n>
inline napi_value ToNode(napi_env env, const char16_t (&value)[n]) {
  return ConvertIgnoringStatus<char16_t[n]>(env, value);
}

// Convert from node and ignore errors.
template<typename T>
inline T FromNodeTo(napi_env env, napi_value value) {
  T result{};
  FromNode(env, value, &result);
  return result;
}

struct DeduceFromNode {
  DeduceFromNode(napi_env env, napi_value value) : env(env), value(value) {}

  template<typename T>
  operator T() { return FromNodeTo<T>(env, value); }

  napi_env env;
  napi_value value;
};

// Define converters for some frequently used types.
template<typename T>
struct Type<std::vector<T>> {
  static constexpr const char* name = "Array";
  static napi_status ToNode(napi_env env,
                            const std::vector<T>& vec,
                            napi_value* result) {
    napi_status s = napi_create_array_with_length(env, vec.size(), result);
    if (s != napi_ok) return s;
    for (size_t i = 0; i < vec.size(); ++i) {
      napi_value el;
      s = ConvertToNode(env, vec[i], &el);
      if (s != napi_ok) return s;
      s = napi_set_element(env, *result, i, el);
      if (s != napi_ok) return s;
    }
    return napi_ok;
  }
  static napi_status FromNode(napi_env env,
                              napi_value value,
                              std::vector<T>* out) {
    if (!IsArray(env, value))
      return napi_array_expected;
    uint32_t length;
    napi_status s = napi_get_array_length(env, value, &length);
    if (s != napi_ok) return s;
    out->resize(length);
    for (uint32_t i = 0; i < length; ++i) {
      napi_value el = nullptr;
      s = napi_get_element(env, value, i, &el);
      if (s != napi_ok) return s;
      s = Type<T>::FromNode(env, el, &(*out)[i]);
      if (s != napi_ok) return s;
    }
    return napi_ok;
  }
};

template<typename T>
struct Type<std::set<T>> {
  static constexpr const char* name = "Array";
  static napi_status ToNode(napi_env env,
                            const std::set<T>& vec,
                            napi_value* result) {
    napi_status s = napi_create_array_with_length(env, vec.size(), result);
    if (s != napi_ok) return s;
    int i = 0;
    for (const auto& element : vec) {
      napi_value el;
      s = ConvertToNode(env, element, &el);
      if (s != napi_ok) return s;
      s = napi_set_element(env, *result, i++, el);
      if (s != napi_ok) return s;
    }
    return napi_ok;
  }
  static napi_status FromNode(napi_env env,
                              napi_value value,
                              std::set<T>* out) {
    if (!IsArray(env, value))
      return napi_array_expected;
    uint32_t length;
    napi_status s = napi_get_array_length(env, value, &length);
    if (s != napi_ok) return s;
    for (uint32_t i = 0; i < length; ++i) {
      napi_value el;
      s = napi_get_element(env, value, i, &el);
      if (s != napi_ok) return s;
      T element;
      s = Type<T>::FromNode(env, el, &element);
      if (s != napi_ok) return s;
      out->insert(std::move(element));
    }
    return napi_ok;
  }
};

template<typename K, typename V>
struct Type<std::map<K, V>> {
  static constexpr const char* name = "Object";
  static napi_status ToNode(napi_env env,
                            const std::map<K, V>& dict,
                            napi_value* result) {
    napi_status s = napi_create_object(env, result);
    if (s == napi_ok) {
      for (const auto& it : dict) {
        napi_value key, value;
        s = ConvertToNode(env, it.first, &key);
        if (s != napi_ok) break;
        s = ConvertToNode(env, it.second, &value);
        if (s != napi_ok) break;
        s = napi_set_property(env, *result, key, value);
        if (s != napi_ok) break;
      }
    }
    return s;
  }
  static napi_status FromNode(napi_env env,
                              napi_value object,
                              std::map<K, V>* out) {
    napi_value property_names;
    napi_status s = napi_get_property_names(env, object, &property_names);
    if (s != napi_ok) return s;
    std::vector<napi_value> keys;
    s = ConvertFromNode(env, property_names, &keys);
    if (s != napi_ok) return s;
    for (napi_value key : keys) {
      K k;
      s = ConvertFromNode(env, key, &k);
      if (s != napi_ok) return s;
      napi_value value;
      s = napi_get_property(env, object, key, &value);
      if (s != napi_ok) return s;
      V v;
      s = ConvertFromNode(env, value, &v);
      if (s != napi_ok) return s;
      out->emplace(std::move(k), std::move(v));
    }
    return napi_ok;
  }
};

namespace internal {

// Helpers used to converting tuple to Array.
inline void SetArray(napi_env, napi_value, int) {
}

template<typename ArgType, typename... ArgTypes>
inline void SetArray(napi_env env,
                     napi_value arr,
                     int i, const ArgType& arg,
                     ArgTypes... args) {
  if (napi_set_element(env, arr, i, ToNode(env, arg)) != napi_ok)
    return;
  SetArray(env, arr, i + 1, args...);
}

template<typename T, size_t... indices>
inline void SetArray(napi_env env,
                     napi_value arr,
                     const T& tup,
                     IndicesHolder<indices...>) {
  SetArray(env, arr, 0, std::get<indices>(tup)...);
}

}  // namespace internal

template<typename... ArgTypes>
struct Type<std::tuple<ArgTypes...>> {
  static constexpr const char* name = "Array";
  static napi_status ToNode(napi_env env,
                            const std::tuple<ArgTypes...>& tup,
                            napi_value* result) {
    constexpr size_t length = sizeof...(ArgTypes);
    napi_value arr;
    napi_status s = napi_create_array_with_length(env, length, &arr);
    if (s == napi_ok)
      SetArray(env, arr, tup,
               typename internal::IndicesGenerator<length>::type());
    return s;
  }
};

}  // namespace ki

#endif  // SRC_TYPES_H_
