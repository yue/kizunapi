// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <kizunapi.h>

namespace {

template<typename T>
T Passthrough(const T& value) {
  return value;
}

}  // namespace

void run_types_tests(napi_env env, napi_value binding) {
  ki::Set(env, binding,
          "value", ki::ToNodeValue(env, "value"),
          "null", nullptr,
          "integer", 123,
          "number", 3.14,
          "bool", false,
          "string", std::string("字符串"),
          "ustring", std::u16string(u"ustring"),
          "charptr", "チャーポインター",
          "ucharptr", u"ucharptr",
          "symbol", ki::Symbol("sym"),
          "tuple", std::tuple<int, bool, std::string>(89, true, "64"),
          "pair", std::pair<std::string, std::string>("a", "pair"),
          "variant", std::variant<bool, int>(8964),
          "map", std::map<std::string, int>{{"123", 456}},
          "passTuple", &Passthrough<std::tuple<int, int>>,
          "passPair", &Passthrough<std::pair<int, int>>,
          "passVariant", &Passthrough<std::variant<float, std::string>>,
          "passMap", &Passthrough<std::map<std::string, int>>);
}
