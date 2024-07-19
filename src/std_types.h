#ifndef SRC_STD_TYPES_H_
#define SRC_STD_TYPES_H_

#include "src/iterator.h"

#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace ki {

template<>
struct Type<std::string> {
  static constexpr const char* name = "String";
  static inline napi_status ToNode(napi_env env,
                                   const std::string& value,
                                   napi_value* result) {
    return napi_create_string_utf8(env, value.c_str(), value.length(), result);
  }
  static std::optional<std::string> FromNode(napi_env env, napi_value value) {
    size_t length;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok)
      return std::nullopt;
    std::string out;
    if (length > 0) {
      out.reserve(length + 1);
      out.resize(length);
      if (napi_get_value_string_utf8(env, value, &out.front(), out.capacity(),
                                     nullptr) != napi_ok) {
        return std::nullopt;
      }
    }
    return out;
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
  static std::optional<std::u16string> FromNode(napi_env env,
                                                napi_value value) {
    size_t length;
    if (napi_get_value_string_utf16(env, value, nullptr, 0, &length) != napi_ok)
      return std::nullopt;
    std::u16string out;
    if (length > 0) {
      out.reserve(length + 1);
      out.resize(length);
      if (napi_get_value_string_utf16(env, value, &out.front(), out.capacity(),
                                      nullptr) != napi_ok) {
        return std::nullopt;
      }
    }
    return out;
  }
};

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
  static std::optional<std::vector<T>> FromNode(napi_env env,
                                                napi_value value) {
    std::vector<T> result;
    if (!IterateArray<T>(env, value,
                         [&](uint32_t i, T value) {
                           result.push_back(std::move(value));
                           return true;
                         })) {
      return std::nullopt;
    }
    return result;
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
  static std::optional<std::set<T>> FromNode(napi_env env,
                                             napi_value value) {
    std::set<T> result;
    if (!IterateArray<T>(env, value,
                         [&](uint32_t i, T value) {
                           result.insert(std::move(value));
                           return true;
                         })) {
      return std::nullopt;
    }
    return result;
  }
};

// Converter for std::map/std::unordered_map.
template<typename T>
struct Type<T, std::enable_if_t<  // is map type
                   std::is_same_v<typename T::value_type,
                                  std::pair<const typename T::key_type,
                                            typename T::mapped_type>>>> {
  using K = typename T::key_type;
  using V = typename T::mapped_type;
  static constexpr const char* name = "Object";
  static napi_status ToNode(napi_env env,
                            const T& dict,
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
  static std::optional<T> FromNode(napi_env env,
                                   napi_value object) {
    T result;
    if (!IterateObject<K, V>(env, object,
                             [&result](K key, V value) {
                               result.emplace(std::move(key), std::move(value));
                               return true;
                             })) {
      return std::nullopt;
    }
    return result;
  }
};

template<typename T>
struct Type<std::optional<T>> {
  static constexpr const char* name = Type<T>::name;
  static napi_status ToNode(napi_env env,
                            const std::optional<T>& value,
                            napi_value* result) {
    if (!value)
      return napi_get_null(env, result);
    return ConvertToNode(env, *value, result);
  }
  static std::optional<std::optional<T>> FromNode(napi_env env,
                                                  napi_value value) {
    napi_valuetype type;
    if (napi_typeof(env, value, &type) != napi_ok)
      return std::nullopt;
    if (type == napi_undefined || type == napi_null)
      return std::optional<T>();
    return Type<T>::FromNode(env, value);
  }
};

template<typename... ArgTypes>
struct Type<std::tuple<ArgTypes...>> {
  using V = std::tuple<ArgTypes...>;

  static constexpr const char* name = "Tuple";
  static napi_status ToNode(napi_env env, const V& tup, napi_value* result) {
    constexpr size_t length = sizeof...(ArgTypes);
    napi_value arr;
    napi_status s = napi_create_array_with_length(env, length, &arr);
    if (s != napi_ok)
      return s;
    s = SetVar(env, arr, tup);
    if (s != napi_ok)
      return s;
    *result = arr;
    return napi_ok;
  }
  static std::optional<V> FromNode(napi_env env, napi_value value) {
    if (!IsArray(env, value))
      return std::nullopt;
    uint32_t length;
    if (napi_get_array_length(env, value, &length) != napi_ok)
      return std::nullopt;
    if (length != sizeof...(ArgTypes))
      return std::nullopt;
    V result;
    if (!GetVar(env, value, &result))
      return std::nullopt;
    return result;
  }

 private:
  template<std::size_t I = 0>
  static napi_status SetVar(napi_env env, napi_value arr, const V& tup) {
    napi_value value;
    napi_status s = ConvertToNode(env, std::get<I>(tup), &value);
    if (s != napi_ok)
      return s;
    s = napi_set_element(env, arr, I, value);
    if (s != napi_ok)
      return s;
    if constexpr (I + 1 >= sizeof...(ArgTypes))
      return napi_ok;
    else
      return SetVar<I + 1>(env, arr, tup);
  }

  template<std::size_t I = 0>
  static bool GetVar(napi_env env, napi_value arr, V* tup) {
    napi_value value;
    if (napi_get_element(env, arr, I, &value) != napi_ok)
      return false;
    using T = std::tuple_element_t<I, V>;
    std::optional<T> result = FromNodeTo<T>(env, value);
    if (!result)
      return false;
    std::get<I>(*tup) = std::move(*result);
    if constexpr (I + 1 >= sizeof...(ArgTypes))
      return true;
    else
      return GetVar<I + 1>(env, arr, tup);
  }
};

template<typename T1, typename T2>
struct Type<std::pair<T1, T2>> {
  using V = std::pair<T1, T2>;
  static constexpr const char* name = "Pair";
  static inline napi_status ToNode(napi_env env,
                                   const V& pair,
                                   napi_value* result) {
    return Type<std::tuple<T1, T2>>::ToNode(env, pair, result);
  }
  static inline std::optional<V> FromNode(napi_env env, napi_value value) {
    auto tup = FromNodeTo<std::tuple<T1, T2>>(env, value);
    if (!tup)
      return std::nullopt;
    return V{std::move(std::get<0>(*tup)), std::move(std::get<1>(*tup))};
  }
};

// Converter for converting std::variant between js and C++.
template<typename... ArgTypes>
struct Type<std::variant<ArgTypes...>> {
  using V = std::variant<ArgTypes...>;

  static constexpr const char* name = "Variant";
  static napi_status ToNode(napi_env env, const V& var, napi_value* result) {
    napi_status s = napi_generic_failure;
    std::visit([env, result, &s](const auto& arg) {
      s = ConvertToNode(env, arg, result);
    }, var);
    return s;
  }
  static inline std::optional<V> FromNode(napi_env env, napi_value value) {
    return GetVar(env, value);
  }

 private:
  template<std::size_t I = 0>
  static std::optional<V> GetVar(napi_env env, napi_value value) {
    if constexpr (I < std::variant_size_v<V>) {
      using T = std::variant_alternative_t<I, V>;
      std::optional<T> result = FromNodeTo<T>(env, value);
      return result ? std::move(*result) : GetVar<I + 1>(env, value);
    }
    return std::nullopt;
  }
};

// The monostate is a special type for std::variant, when a variant includes it,
// we consider the type accepting undefined/null.
template<>
struct Type<std::monostate> {
  static constexpr const char* name = "";  // no name for monostate
  static napi_status ToNode(napi_env env, std::monostate, napi_value* result) {
    return napi_get_null(env, result);
  }
  static inline std::optional<std::monostate> FromNode(napi_env env,
                                                       napi_value value) {
    napi_valuetype type;
    if (napi_typeof(env, value, &type) != napi_ok)
      return std::nullopt;
    if (type == napi_undefined || type == napi_null)
      return std::monostate();
    return std::nullopt;
  }
};

}  // namespace ki

#endif  // SRC_STD_TYPES_H_
