#include <limits.h>  // INT_MAX

#if !defined(__wasm__) || (defined(__EMSCRIPTEN__) || defined(__wasi__))
#include <stdlib.h>
#include <string.h>
#else
void *memcpy(void * dest, const void * src, size_t n);
void* memset(void* dst, int c, size_t n);
void* malloc(size_t size);
void free(void* p);

int memcmp(const void *vl, const void *vr, size_t n)
{
	const unsigned char *l=vl, *r=vr;
	for (; n && *l == *r; n--, l++, r++);
	return n ? *l-*r : 0;
}

size_t strlen(const char *s)
{
	const char *a = s;
	for (; *s; s++);
	return s-a;
}

void abort() {
  __builtin_trap();
}
#endif

#define NAPI_EXPERIMENTAL
#include <js_native_api.h>
#include "../common.h"
#include "test_null.h"

enum length_type { actual_length, auto_length };

static napi_status validate_and_retrieve_single_string_arg(
    napi_env env, napi_callback_info info, napi_value* arg) {
  size_t argc = 1;
  NAPI_CHECK_STATUS(napi_get_cb_info(env, info, &argc, arg, NULL, NULL));

  NAPI_ASSERT_STATUS(env, argc >= 1, "Wrong number of arguments");

  napi_valuetype valuetype;
  NAPI_CHECK_STATUS(napi_typeof(env, *arg, &valuetype));

  NAPI_ASSERT_STATUS(env,
                         valuetype == napi_string,
                         "Wrong type of argment. Expects a string.");

  return napi_ok;
}

// These help us factor out code that is common between the bindings.
typedef napi_status (*OneByteCreateAPI)(napi_env,
                                        const char*,
                                        size_t,
                                        napi_value*);
typedef napi_status (*OneByteGetAPI)(
    napi_env, napi_value, char*, size_t, size_t*);
typedef napi_status (*TwoByteCreateAPI)(napi_env,
                                        const char16_t*,
                                        size_t,
                                        napi_value*);
typedef napi_status (*TwoByteGetAPI)(
    napi_env, napi_value, char16_t*, size_t, size_t*);

// Test passing back the one-byte string we got from JS.
static napi_value TestOneByteImpl(napi_env env,
                                  napi_callback_info info,
                                  OneByteGetAPI get_api,
                                  OneByteCreateAPI create_api,
                                  enum length_type length_mode) {
  napi_value args[1];
  NAPI_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char buffer[128];
  size_t buffer_size = 128;
  size_t copied;

  NAPI_CALL(env, get_api(env, args[0], buffer, buffer_size, &copied));

  napi_value output;
  if (length_mode == auto_length) {
    copied = NAPI_AUTO_LENGTH;
  }
  NAPI_CALL(env, create_api(env, buffer, copied, &output));

  return output;
}

// Test passing back the two-byte string we got from JS.
static napi_value TestTwoByteImpl(napi_env env,
                                  napi_callback_info info,
                                  TwoByteGetAPI get_api,
                                  TwoByteCreateAPI create_api,
                                  enum length_type length_mode) {
  napi_value args[1];
  NAPI_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char16_t buffer[128];
  size_t buffer_size = 128;
  size_t copied;

  NAPI_CALL(env, get_api(env, args[0], buffer, buffer_size, &copied));

  napi_value output;
  if (length_mode == auto_length) {
    copied = NAPI_AUTO_LENGTH;
  }
  NAPI_CALL(env, create_api(env, buffer, copied, &output));

  return output;
}

static void free_string(napi_env env, void* data, void* hint) {
  free(data);
}

static napi_status create_external_latin1(napi_env env,
                                          const char* string,
                                          size_t length,
                                          napi_value* result) {
  napi_status status;
  // Initialize to false, because that is the value we don't want.
  bool copied = false;
  char* string_copy;
  const size_t actual_length =
      (length == NAPI_AUTO_LENGTH ? strlen(string) : length);
  const size_t length_bytes = (actual_length + 1) * sizeof(*string_copy);
  string_copy = malloc(length_bytes);
  memcpy(string_copy, string, length_bytes);
  string_copy[actual_length] = 0;

  status = node_api_create_external_string_latin1(
      env, string_copy, length, free_string, NULL, result, &copied);
  // We do not want the string to be copied.
  if (!copied) {
    return napi_generic_failure;
  }
  if (status != napi_ok) {
    free(string_copy);
    return status;
  }
  return napi_ok;
}

// strlen for char16_t. Needed in case we're copying a string of length
// NAPI_AUTO_LENGTH.
static size_t strlen16(const char16_t* string) {
  for (const char16_t* iter = string;; iter++) {
    if (*iter == 0) {
      return iter - string;
    }
  }
  // We should never get here.
  abort();
}

static napi_status create_external_utf16(napi_env env,
                                         const char16_t* string,
                                         size_t length,
                                         napi_value* result) {
  napi_status status;
  // Initialize to false, because that is the value we don't want.
  bool copied = false;
  char16_t* string_copy;
  const size_t actual_length =
      (length == NAPI_AUTO_LENGTH ? strlen16(string) : length);
  const size_t length_bytes = (actual_length + 1) * sizeof(*string_copy);
  string_copy = malloc(length_bytes);
  memcpy(string_copy, string, length_bytes);
  string_copy[actual_length] = 0;

  status = node_api_create_external_string_utf16(
      env, string_copy, length, free_string, NULL, result, &copied);
  if (status != napi_ok) {
    free(string_copy);
    return status;
  }

  return napi_ok;
}

static napi_value TestLatin1(napi_env env, napi_callback_info info) {
  return TestOneByteImpl(env,
                         info,
                         napi_get_value_string_latin1,
                         napi_create_string_latin1,
                         actual_length);
}

static napi_value TestUtf8(napi_env env, napi_callback_info info) {
  return TestOneByteImpl(env,
                         info,
                         napi_get_value_string_utf8,
                         napi_create_string_utf8,
                         actual_length);
}

static napi_value TestUtf16(napi_env env, napi_callback_info info) {
  return TestTwoByteImpl(env,
                         info,
                         napi_get_value_string_utf16,
                         napi_create_string_utf16,
                         actual_length);
}

static napi_value TestLatin1AutoLength(napi_env env, napi_callback_info info) {
  return TestOneByteImpl(env,
                         info,
                         napi_get_value_string_latin1,
                         napi_create_string_latin1,
                         auto_length);
}

static napi_value TestUtf8AutoLength(napi_env env, napi_callback_info info) {
  return TestOneByteImpl(env,
                         info,
                         napi_get_value_string_utf8,
                         napi_create_string_utf8,
                         auto_length);
}

static napi_value TestUtf16AutoLength(napi_env env, napi_callback_info info) {
  return TestTwoByteImpl(env,
                         info,
                         napi_get_value_string_utf16,
                         napi_create_string_utf16,
                         auto_length);
}

static napi_value TestLatin1External(napi_env env, napi_callback_info info) {
  return TestOneByteImpl(env,
                         info,
                         napi_get_value_string_latin1,
                         create_external_latin1,
                         actual_length);
}

static napi_value TestUtf16External(napi_env env, napi_callback_info info) {
  return TestTwoByteImpl(env,
                         info,
                         napi_get_value_string_utf16,
                         create_external_utf16,
                         actual_length);
}

static napi_value TestLatin1ExternalAutoLength(napi_env env,
                                               napi_callback_info info) {
  return TestOneByteImpl(env,
                         info,
                         napi_get_value_string_latin1,
                         create_external_latin1,
                         auto_length);
}

static napi_value TestUtf16ExternalAutoLength(napi_env env,
                                              napi_callback_info info) {
  return TestTwoByteImpl(env,
                         info,
                         napi_get_value_string_utf16,
                         create_external_utf16,
                         auto_length);
}

static napi_value TestLatin1Insufficient(napi_env env,
                                         napi_callback_info info) {
  napi_value args[1];
  NAPI_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char buffer[4];
  size_t buffer_size = 4;
  size_t copied;

  NAPI_CALL(
      env,
      napi_get_value_string_latin1(env, args[0], buffer, buffer_size, &copied));

  napi_value output;
  NAPI_CALL(env, napi_create_string_latin1(env, buffer, copied, &output));

  return output;
}

static napi_value TestUtf8Insufficient(napi_env env, napi_callback_info info) {
  napi_value args[1];
  NAPI_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char buffer[4];
  size_t buffer_size = 4;
  size_t copied;

  NAPI_CALL(
      env,
      napi_get_value_string_utf8(env, args[0], buffer, buffer_size, &copied));

  napi_value output;
  NAPI_CALL(env, napi_create_string_utf8(env, buffer, copied, &output));

  return output;
}

static napi_value TestUtf16Insufficient(napi_env env, napi_callback_info info) {
  napi_value args[1];
  NAPI_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char16_t buffer[4];
  size_t buffer_size = 4;
  size_t copied;

  NAPI_CALL(
      env,
      napi_get_value_string_utf16(env, args[0], buffer, buffer_size, &copied));

  napi_value output;
  NAPI_CALL(env, napi_create_string_utf16(env, buffer, copied, &output));

  return output;
}

static napi_value Utf16Length(napi_env env, napi_callback_info info) {
  napi_value args[1];
  NAPI_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  size_t length;
  NAPI_CALL(env,
                napi_get_value_string_utf16(env, args[0], NULL, 0, &length));

  napi_value output;
  NAPI_CALL(env, napi_create_uint32(env, (uint32_t)length, &output));

  return output;
}

static napi_value Utf8Length(napi_env env, napi_callback_info info) {
  napi_value args[1];
  NAPI_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  size_t length;
  NAPI_CALL(env,
                napi_get_value_string_utf8(env, args[0], NULL, 0, &length));

  napi_value output;
  NAPI_CALL(env, napi_create_uint32(env, (uint32_t)length, &output));

  return output;
}

static napi_value TestLargeUtf8(napi_env env, napi_callback_info info) {
  napi_value output;
  if (SIZE_MAX > INT_MAX) {
    NAPI_CALL(
        env, napi_create_string_utf8(env, "", ((size_t)INT_MAX) + 1, &output));
  } else {
    // just throw the expected error as there is nothing to test
    // in this case since we can't overflow
    NAPI_CALL(env, napi_throw_error(env, NULL, "Invalid argument"));
  }

  return output;
}

static napi_value TestLargeLatin1(napi_env env, napi_callback_info info) {
  napi_value output;
  if (SIZE_MAX > INT_MAX) {
    NAPI_CALL(
        env,
        napi_create_string_latin1(env, "", ((size_t)INT_MAX) + 1, &output));
  } else {
    // just throw the expected error as there is nothing to test
    // in this case since we can't overflow
    NAPI_CALL(env, napi_throw_error(env, NULL, "Invalid argument"));
  }

  return output;
}

static napi_value TestLargeUtf16(napi_env env, napi_callback_info info) {
  napi_value output;
  if (SIZE_MAX > INT_MAX) {
    NAPI_CALL(
        env,
        napi_create_string_utf16(
            env, ((const char16_t*)""), ((size_t)INT_MAX) + 1, &output));
  } else {
    // just throw the expected error as there is nothing to test
    // in this case since we can't overflow
    NAPI_CALL(env, napi_throw_error(env, NULL, "Invalid argument"));
  }

  return output;
}

static napi_value TestMemoryCorruption(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

  NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

  char buf[10] = {0};
  NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buf, 0, NULL));

  char zero[10] = {0};
  if (memcmp(buf, zero, sizeof(buf)) != 0) {
    NAPI_CALL(env, napi_throw_error(env, NULL, "Buffer overwritten"));
  }

  return NULL;
}

static napi_value TestUtf8Large(napi_env env, napi_callback_info info) {
  size_t size = 256 * 1024 * 1024;
  char* buffer = (char*)malloc(size);
  memset(buffer, 97, size);

  napi_value output;
  NAPI_CALL(env, napi_create_string_utf8(env, buffer, size, &output));
  free(buffer);

  return output;
}

static napi_value TestUtf16Large(napi_env env, napi_callback_info info) {
  size_t size = 64 * 1024 * 1024;
  uint16_t* buffer = (uint16_t*)malloc(size * sizeof(uint16_t));
  memset(buffer, 97, size * sizeof(uint16_t));

  napi_value output;
  NAPI_CALL(env, napi_create_string_utf16(env, buffer, size, &output));
  free(buffer);

  return output;
}

EXTERN_C_START
napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_PROPERTY("TestLatin1", TestLatin1),
      DECLARE_NAPI_PROPERTY("TestLatin1AutoLength", TestLatin1AutoLength),
      DECLARE_NAPI_PROPERTY("TestLatin1External", TestLatin1External),
      DECLARE_NAPI_PROPERTY("TestLatin1ExternalAutoLength",
                                TestLatin1ExternalAutoLength),
      DECLARE_NAPI_PROPERTY("TestLatin1Insufficient",
                                TestLatin1Insufficient),
      DECLARE_NAPI_PROPERTY("TestUtf8", TestUtf8),
      DECLARE_NAPI_PROPERTY("TestUtf8AutoLength", TestUtf8AutoLength),
      DECLARE_NAPI_PROPERTY("TestUtf8Insufficient", TestUtf8Insufficient),
      DECLARE_NAPI_PROPERTY("TestUtf16", TestUtf16),
      DECLARE_NAPI_PROPERTY("TestUtf16AutoLength", TestUtf16AutoLength),
      DECLARE_NAPI_PROPERTY("TestUtf16External", TestUtf16External),
      DECLARE_NAPI_PROPERTY("TestUtf16ExternalAutoLength",
                                TestUtf16ExternalAutoLength),
      DECLARE_NAPI_PROPERTY("TestUtf16Insufficient", TestUtf16Insufficient),
      DECLARE_NAPI_PROPERTY("Utf16Length", Utf16Length),
      DECLARE_NAPI_PROPERTY("Utf8Length", Utf8Length),
      DECLARE_NAPI_PROPERTY("TestLargeUtf8", TestLargeUtf8),
      DECLARE_NAPI_PROPERTY("TestLargeLatin1", TestLargeLatin1),
      DECLARE_NAPI_PROPERTY("TestLargeUtf16", TestLargeUtf16),
      DECLARE_NAPI_PROPERTY("TestMemoryCorruption", TestMemoryCorruption),
      DECLARE_NAPI_PROPERTY("TestUtf8Large", TestUtf8Large),
      DECLARE_NAPI_PROPERTY("TestUtf16Large", TestUtf16Large),
  };

  init_test_null(env, exports);

  NAPI_CALL(
      env,
      napi_define_properties(
          env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}
EXTERN_C_END
