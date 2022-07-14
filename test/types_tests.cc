// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <kizunapi.h>

void run_types_tests(napi_env env, napi_value binding) {
  ki::Set(env, binding,
          "value", ki::ToNode(env, "value"),
          "null", nullptr,
          "integer", 123,
          "number", 3.14,
          "bool", false,
          "string", std::string("字符串"),
          "ustring", std::u16string(u"ustring"),
          "charptr", "チャーポインター",
          "ucharptr", u"ucharptr",
          "symbol", ki::Symbol("sym"));
}
