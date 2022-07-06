// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <nbind.h>

namespace {

int Method() {
  return 8964;
}

int number = 19890604;

int Getter() {
  return number;
}

void Setter(int n) {
  number = n + 1;
}

}  // namespace

void run_property_tests(napi_env env, napi_value binding) {
  nb::DefineProperties(
      env, binding,
      nb::Property("value", nb::ToNode(env, "value")),
      nb::Property("method", nb::Method(&Method)),
      nb::Property("number", nb::Getter(&Getter), nb::Setter(&Setter)));
}
