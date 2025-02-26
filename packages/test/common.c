#include <js_native_api.h>
#include "common.h"

#if !defined(__wasm__) || (defined(__EMSCRIPTEN__) || defined(__wasi__))
#include <stdio.h>
#endif

void add_returned_status(napi_env env,
                         const char* key,
                         napi_value object,
                         char* expected_message,
                         napi_status expected_status,
                         napi_status actual_status) {

  char napi_message_string[100] = "";
  napi_value prop_value;
#if !defined(__wasm__) || (defined(__EMSCRIPTEN__) || defined(__wasi__))
  if (actual_status != expected_status) {
    snprintf(napi_message_string, sizeof(napi_message_string),
        "Invalid status [%d]", actual_status);
  }
#endif

  NAPI_CALL_RETURN_VOID(env,
      napi_create_string_utf8(
                              env,
                              (actual_status == expected_status ?
                                  expected_message :
                                  napi_message_string),
                              NAPI_AUTO_LENGTH,
                              &prop_value));
  NAPI_CALL_RETURN_VOID(env,
      napi_set_named_property(env, object, key, prop_value));
}

void add_last_status(napi_env env, const char* key, napi_value return_value) {
    napi_value prop_value;
    const napi_extended_error_info* p_last_error;
    NAPI_CALL_RETURN_VOID(env,
        napi_get_last_error_info(env, &p_last_error));

    NAPI_CALL_RETURN_VOID(env,
        napi_create_string_utf8(env,
                                (p_last_error->error_message == NULL ?
                                    "napi_ok" :
                                    p_last_error->error_message),
                                NAPI_AUTO_LENGTH,
                                &prop_value));
    NAPI_CALL_RETURN_VOID(env,
        napi_set_named_property(env, return_value, key, prop_value));
}
