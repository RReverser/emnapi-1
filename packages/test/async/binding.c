#if !defined(__wasm__) || (defined(__EMSCRIPTEN__) || defined(__wasi__))
#include <stdio.h>
#else
int console_log(const char* fmt, void* a);
#endif
#include <node_api.h>
#include "../common.h"

#if defined _WIN32
#include <windows.h>
#else
#if !defined(__wasm__) || (defined(__EMSCRIPTEN__) || defined(__wasi__))
#include <unistd.h>
#else
void sleep(int s);
#endif
#endif

// this needs to be greater than the thread pool size
#define MAX_CANCEL_THREADS 6

typedef struct {
  int32_t _input;
  int32_t _output;
  napi_ref _callback;
  napi_async_work _request;
} carrier;

static carrier the_carrier;
static carrier async_carrier[MAX_CANCEL_THREADS];

static void Execute(napi_env env, void* data) {
#if defined _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif
  carrier* c = (carrier*)(data);

  if (c != &the_carrier) {
    __builtin_trap();
  }

  c->_output = c->_input * 2;
}

static void Complete(napi_env env, napi_status status, void* data) {
  carrier* c = (carrier*)(data);

  if (c != &the_carrier) {
    napi_throw_type_error(env, NULL, "Wrong data parameter to Complete.");
    return;
  }

  if (status != napi_ok) {
    napi_throw_type_error(env, NULL, "Execute callback failed.");
    return;
  }

  napi_value argv[2];

  NAPI_CALL_RETURN_VOID(env, napi_get_null(env, &argv[0]));
  NAPI_CALL_RETURN_VOID(env, napi_create_int32(env, c->_output, &argv[1]));
  napi_value callback;
  NAPI_CALL_RETURN_VOID(env,
      napi_get_reference_value(env, c->_callback, &callback));
  napi_value global;
  NAPI_CALL_RETURN_VOID(env, napi_get_global(env, &global));

  napi_value result;
  NAPI_CALL_RETURN_VOID(env,
    napi_call_function(env, global, callback, 2, argv, &result));

  NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, c->_callback));
  NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, c->_request));
}

static napi_value Test(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv[3];
  napi_value _this;
  napi_value resource_name;
  void* data;
  NAPI_CALL(env,
    napi_get_cb_info(env, info, &argc, argv, &_this, &data));
  NAPI_ASSERT(env, argc >= 3, "Not enough arguments, expected 2.");

  napi_valuetype t;
  NAPI_CALL(env, napi_typeof(env, argv[0], &t));
  NAPI_ASSERT(env, t == napi_number,
      "Wrong first argument, integer expected.");
  NAPI_CALL(env, napi_typeof(env, argv[1], &t));
  NAPI_ASSERT(env, t == napi_object,
    "Wrong second argument, object expected.");
  NAPI_CALL(env, napi_typeof(env, argv[2], &t));
  NAPI_ASSERT(env, t == napi_function,
    "Wrong third argument, function expected.");

  the_carrier._output = 0;

  NAPI_CALL(env,
      napi_get_value_int32(env, argv[0], &the_carrier._input));
  NAPI_CALL(env,
    napi_create_reference(env, argv[2], 1, &the_carrier._callback));

  NAPI_CALL(env, napi_create_string_utf8(
      env, "TestResource", NAPI_AUTO_LENGTH, &resource_name));
  NAPI_CALL(env, napi_create_async_work(env, argv[1], resource_name,
    Execute, Complete, &the_carrier, &the_carrier._request));
  NAPI_CALL(env,
      napi_queue_async_work(env, the_carrier._request));

  return NULL;
}

static void BusyCancelComplete(napi_env env, napi_status status, void* data) {
  carrier* c = (carrier*)(data);
  NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, c->_request));
}

static void CancelComplete(napi_env env, napi_status status, void* data) {
  carrier* c = (carrier*)(data);

  if (status == napi_cancelled) {
    // ok we got the status we expected so make the callback to
    // indicate the cancel succeeded.
    napi_value callback;
    NAPI_CALL_RETURN_VOID(env,
        napi_get_reference_value(env, c->_callback, &callback));
    napi_value global;
    NAPI_CALL_RETURN_VOID(env, napi_get_global(env, &global));
    napi_value result;
    NAPI_CALL_RETURN_VOID(env,
      napi_call_function(env, global, callback, 0, NULL, &result));
  }

  NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, c->_request));
  NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, c->_callback));
}

static void CancelExecute(napi_env env, void* data) {
#if defined _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif

#if !defined(__wasm__) || (defined(__EMSCRIPTEN__) || defined(__wasi__))
  printf("CancelExecute: %p\n", data);
#else
  console_log("CancelExecute: %d", data);
#endif
}

static napi_value TestCancel(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_value _this;
  napi_value resource_name;
  void* data;

  NAPI_CALL(env, napi_create_string_utf8(
      env, "TestResource", NAPI_AUTO_LENGTH, &resource_name));

  // make sure the work we are going to cancel will not be
  // able to start by using all the threads in the pool
  for (int i = 1; i < MAX_CANCEL_THREADS; i++) {
    NAPI_CALL(env, napi_create_async_work(env, NULL, resource_name,
      CancelExecute, BusyCancelComplete,
      &async_carrier[i], &async_carrier[i]._request));
    NAPI_CALL(env, napi_queue_async_work(env, async_carrier[i]._request));
  }

  // now queue the work we are going to cancel and then cancel it.
  // cancel will fail if the work has already started, but
  // we have prevented it from starting by consuming all of the
  // workers above.
  NAPI_CALL(env,
    napi_get_cb_info(env, info, &argc, argv, &_this, &data));
  NAPI_CALL(env, napi_create_async_work(env, NULL, resource_name,
    CancelExecute, CancelComplete,
    &async_carrier[0], &async_carrier[0]._request));
  NAPI_CALL(env,
      napi_create_reference(env, argv[0], 1, &async_carrier[0]._callback));
  NAPI_CALL(env, napi_queue_async_work(env, async_carrier[0]._request));
  NAPI_CALL(env, napi_cancel_async_work(env, async_carrier[0]._request));
  return NULL;
}

struct {
  napi_ref ref;
  napi_async_work work;
} repeated_work_info = { NULL, NULL };

static void RepeatedWorkerThread(napi_env env, void* data) {}

static void RepeatedWorkComplete(napi_env env, napi_status status, void* data) {
  napi_value cb, js_status;
  NAPI_CALL_RETURN_VOID(env,
      napi_get_reference_value(env, repeated_work_info.ref, &cb));
  NAPI_CALL_RETURN_VOID(env,
      napi_delete_async_work(env, repeated_work_info.work));
  NAPI_CALL_RETURN_VOID(env,
      napi_delete_reference(env, repeated_work_info.ref));
  repeated_work_info.work = NULL;
  repeated_work_info.ref = NULL;
  NAPI_CALL_RETURN_VOID(env,
      napi_create_uint32(env, (uint32_t)status, &js_status));
  NAPI_CALL_RETURN_VOID(env,
      napi_call_function(env, cb, cb, 1, &js_status, NULL));
}

static napi_value DoRepeatedWork(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value cb, name;
  NAPI_ASSERT(env, repeated_work_info.ref == NULL,
      "Reference left over from previous work");
  NAPI_ASSERT(env, repeated_work_info.work == NULL,
      "Work pointer left over from previous work");
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &cb, NULL, NULL));
  NAPI_CALL(env, napi_create_reference(env, cb, 1, &repeated_work_info.ref));
  NAPI_CALL(env,
      napi_create_string_utf8(env, "Repeated Work", NAPI_AUTO_LENGTH, &name));
  NAPI_CALL(env,
      napi_create_async_work(env, NULL, name, RepeatedWorkerThread,
          RepeatedWorkComplete, &repeated_work_info, &repeated_work_info.work));
  NAPI_CALL(env, napi_queue_async_work(env, repeated_work_info.work));
  return NULL;
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
    DECLARE_NAPI_PROPERTY("Test", Test),
    DECLARE_NAPI_PROPERTY("TestCancel", TestCancel),
    DECLARE_NAPI_PROPERTY("DoRepeatedWork", DoRepeatedWork),
  };

  NAPI_CALL(env, napi_define_properties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
