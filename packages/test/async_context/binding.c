#include <node_api.h>
#include "../common.h"

#define assert(x) do { if (!(x)) { __builtin_trap(); } } while (0)

#define MAX_ARGUMENTS 10
#define RESERVED_ARGS 3

static napi_value MakeCallback(napi_env env, napi_callback_info info) {
  size_t argc = MAX_ARGUMENTS;
  size_t n;
  napi_value args[MAX_ARGUMENTS];
  // NOLINTNEXTLINE (readability/null_usage)
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

  NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

  napi_value async_context_wrap = args[0];
  napi_value recv = args[1];
  napi_value func = args[2];

  napi_value argv[MAX_ARGUMENTS - RESERVED_ARGS];
  for (n = RESERVED_ARGS; n < argc; n += 1) {
    argv[n - RESERVED_ARGS] = args[n];
  }

  napi_valuetype func_type;
  NAPI_CALL(env, napi_typeof(env, func, &func_type));

  napi_async_context context;
  NAPI_CALL(env, napi_unwrap(env, async_context_wrap, (void**)&context));

  napi_value result;
  if (func_type == napi_function) {
    NAPI_CALL(env, napi_make_callback(
        env, context, recv, func, argc - RESERVED_ARGS, argv, &result));
  } else {
    NAPI_ASSERT(env, false, "Unexpected argument type");
  }

  return result;
}

static void AsyncDestroyCb(napi_env env, void* data, void* hint) {
  napi_status status = napi_async_destroy(env, (napi_async_context)data);
  // We cannot use NAPI_ASSERT_RETURN_VOID because we need to have a JS
  // stack below in order to use exceptions.
  assert(status == napi_ok);
}

#define CREATE_ASYNC_RESOURCE_ARGC 2

static napi_value CreateAsyncResource(napi_env env, napi_callback_info info) {
  napi_value async_context_wrap;
  NAPI_CALL(env, napi_create_object(env, &async_context_wrap));

  size_t argc = CREATE_ASYNC_RESOURCE_ARGC;
  napi_value args[CREATE_ASYNC_RESOURCE_ARGC];
  // NOLINTNEXTLINE (readability/null_usage)
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

  napi_value resource = args[0];
  napi_value js_destroy_on_finalizer = args[1];
  napi_valuetype resource_type;
  NAPI_CALL(env, napi_typeof(env, resource, &resource_type));
  if (resource_type != napi_object) {
    resource = NULL;
  }

  napi_value resource_name;
  NAPI_CALL(env, napi_create_string_utf8(
      env, "test_async", NAPI_AUTO_LENGTH, &resource_name));

  napi_async_context context;
  NAPI_CALL(env, napi_async_init(env, resource, resource_name, &context));

  bool destroy_on_finalizer = true;
  if (argc == 2) {
    NAPI_CALL(env, napi_get_value_bool(env, js_destroy_on_finalizer, &destroy_on_finalizer));
  }
  if (resource_type == napi_object && destroy_on_finalizer) {
    NAPI_CALL(env, napi_add_finalizer(
        env, resource, (void*)context, AsyncDestroyCb, NULL, NULL));
  }
  NAPI_CALL(env, napi_wrap(env, async_context_wrap, context, NULL, NULL, NULL));
  return async_context_wrap;
}

#define DESTROY_ASYNC_RESOURCE_ARGC 1

static napi_value DestroyAsyncResource(napi_env env, napi_callback_info info) {
  size_t argc = DESTROY_ASYNC_RESOURCE_ARGC;
  napi_value args[DESTROY_ASYNC_RESOURCE_ARGC];
  // NOLINTNEXTLINE (readability/null_usage)
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
  NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

  napi_value async_context_wrap = args[0];

  napi_async_context async_context;
  NAPI_CALL(env,
            napi_remove_wrap(env, async_context_wrap, (void**)&async_context));
  NAPI_CALL(env, napi_async_destroy(env, async_context));

  return async_context_wrap;
}

static
napi_value Init(napi_env env, napi_value exports) {
  napi_value fn;
  NAPI_CALL(env, napi_create_function(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, NAPI_AUTO_LENGTH, MakeCallback, NULL, &fn));
  NAPI_CALL(env, napi_set_named_property(env, exports, "makeCallback", fn));
  NAPI_CALL(env, napi_create_function(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, NAPI_AUTO_LENGTH, CreateAsyncResource, NULL, &fn));
  NAPI_CALL(env, napi_set_named_property(
      env, exports, "createAsyncResource", fn));

  NAPI_CALL(env, napi_create_function(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, NAPI_AUTO_LENGTH, DestroyAsyncResource, NULL, &fn));
  NAPI_CALL(env, napi_set_named_property(
      env, exports, "destroyAsyncResource", fn));

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
