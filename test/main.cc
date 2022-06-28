#include <node_api.h>

#include "types.h"

napi_value Init(napi_env env, napi_value exports) {
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
