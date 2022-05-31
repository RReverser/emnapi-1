#include <emscripten.h>
#include "emnapi.h"
#include "node_api.h"

#define CHECK_ENV(env)          \
  do {                          \
    if ((env) == NULL) {        \
      return napi_invalid_arg;  \
    }                           \
  } while (0)

#define RETURN_STATUS_IF_FALSE(env, condition, status)                  \
  do {                                                                  \
    if (!(condition)) {                                                 \
      return napi_set_last_error((env), (status), 0, NULL);             \
    }                                                                   \
  } while (0)

#define CHECK_ARG(env, arg) \
  RETURN_STATUS_IF_FALSE((env), ((arg) != NULL), napi_invalid_arg)

EXTERN_C_START

struct instance_data_s {
  void* data;
  napi_finalize finalize_cb;
  void* finalize_hint;
};

struct napi_env__ {
  napi_extended_error_info last_error;
  struct instance_data_s* instance_data;
};

napi_status napi_clear_last_error(napi_env env) {
  env->last_error.error_code = napi_ok;

  env->last_error.engine_error_code = 0;
  env->last_error.engine_reserved = NULL;
  env->last_error.error_message = NULL;
  return napi_ok;
}

napi_status napi_set_last_error(napi_env env, napi_status error_code,
                                uint32_t engine_error_code,
                                void* engine_reserved) {
  env->last_error.error_code = error_code;
  env->last_error.engine_error_code = engine_error_code;
  env->last_error.engine_reserved = engine_reserved;
  return error_code;
}

napi_status napi_set_instance_data(napi_env env,
                                   void* data,
                                   napi_finalize finalize_cb,
                                   void* finalize_hint) {
  CHECK_ENV(env);

  struct instance_data_s* old_data = env->instance_data;
  if (old_data != NULL) {
    if (old_data->finalize_cb != NULL) {
      old_data->finalize_cb(env, old_data->data, old_data->finalize_hint);
    }
    free(old_data);
  }

  env->instance_data = (struct instance_data_s*)
    malloc(sizeof(struct instance_data_s));
  env->instance_data->data = data;
  env->instance_data->finalize_cb = finalize_cb;
  env->instance_data->finalize_hint = finalize_hint;

  return napi_clear_last_error(env);
}

napi_status napi_get_instance_data(napi_env env,
                                   void** data) {
  CHECK_ENV(env);
  CHECK_ARG(env, data);

  struct instance_data_s* idata = env->instance_data;

  *data = (idata == NULL ? NULL : idata->data);

  return napi_clear_last_error(env);
}

static const char* error_messages[] = {
  NULL,
  "Invalid argument",
  "An object was expected",
  "A string was expected",
  "A string or symbol was expected",
  "A function was expected",
  "A number was expected",
  "A boolean was expected",
  "An array was expected",
  "Unknown failure",
  "An exception is pending",
  "The async work item was cancelled",
  "napi_escape_handle already called on scope",
  "Invalid handle scope usage",
  "Invalid callback scope usage",
  "Thread-safe function queue is full",
  "Thread-safe function handle is closing",
  "A bigint was expected",
  "A date was expected",
  "An arraybuffer was expected",
  "A detachable arraybuffer was expected",
  "Main thread would deadlock",
};

napi_status
napi_get_last_error_info(napi_env env,
                         const napi_extended_error_info** result) {
  CHECK_ENV(env);
  CHECK_ARG(env, result);

  env->last_error.error_message =
    error_messages[env->last_error.error_code];

  if (env->last_error.error_code == napi_ok) {
    napi_clear_last_error(env);
  }
  *result = &(env->last_error);
  return napi_ok;
}

napi_status
napi_get_node_version(napi_env env,
                      const napi_node_version** version) {
  if (env == NULL) return napi_invalid_arg;
  if (version == NULL) {
    return napi_set_last_error(env, napi_invalid_arg, 0, NULL);
  }
  static napi_node_version node_version = {
    16,
    15,
    0,
    "node"
  };
  *version = &node_version;
  return napi_clear_last_error(env);
}

napi_status
emnapi_get_emscripten_version(napi_env env,
                              const emnapi_emscripten_version** version) {
  if (env == NULL) return napi_invalid_arg;
  if (version == NULL) {
    return napi_set_last_error(env, napi_invalid_arg, 0, NULL);
  }
  static emnapi_emscripten_version emscripten_version = {
    __EMSCRIPTEN_major__,
    __EMSCRIPTEN_minor__,
    __EMSCRIPTEN_tiny__
  };
  *version = &emscripten_version;
  return napi_clear_last_error(env);
}

napi_status napi_adjust_external_memory(napi_env env,
                                        int64_t change_in_bytes,
                                        int64_t* adjusted_value) {
  return napi_set_last_error(env, napi_generic_failure, 0, NULL);
}

napi_status
napi_create_external_arraybuffer(napi_env env,
                                 void* external_data,
                                 size_t byte_length,
                                 napi_finalize finalize_cb,
                                 void* finalize_hint,
                                 napi_value* result) {
  return napi_set_last_error(env, napi_generic_failure, 0, NULL);
}

#if NAPI_VERSION >= 7
napi_status napi_detach_arraybuffer(napi_env env,
                                    napi_value arraybuffer) {
  return napi_set_last_error(env, napi_generic_failure, 0, NULL);
}
#endif

EXTERN_C_END
