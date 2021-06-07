function napi_get_last_error_info (env: napi_env, result: Pointer<Pointer<napi_extended_error_info>>): emnapi.napi_status {
  if (!emnapi.checkEnv(env)) return emnapi.napi_status.napi_invalid_arg
  if (result === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)
  const envObject = emnapi.envStore.get(env)!
  envObject.napiExtendedErrorInfo.error_message = emnapi.errorMessagesPtr[envObject.napiExtendedErrorInfo.error_code]
  HEAP32[envObject.napiExtendedErrorInfoPtr >> 2] = envObject.napiExtendedErrorInfo.error_message

  HEAP32[result >> 2] = envObject.napiExtendedErrorInfoPtr
  return emnapi.napi_status.napi_ok
}

function napi_throw (env: napi_env, error: napi_value): emnapi.napi_status {
  if (!emnapi.checkEnv(env)) return emnapi.napi_status.napi_invalid_arg
  const envObject = emnapi.envStore.get(env)!
  if (envObject.tryCatch.hasCaught()) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  emnapi.napi_clear_last_error(env)
  if (error === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)
  try {
    envObject.tryCatch.setError(envObject.handleStore.get(error)!.value)
  } catch (err) {
    envObject.tryCatch.setError(err)
    return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  }
  return emnapi.napi_clear_last_error(env)
}

function napi_throw_error (env: napi_env, code: const_char_p, msg: const_char_p): emnapi.napi_status {
  if (!emnapi.checkEnv(env)) return emnapi.napi_status.napi_invalid_arg
  const envObject = emnapi.envStore.get(env)!
  if (envObject.tryCatch.hasCaught()) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  emnapi.napi_clear_last_error(env)
  if (msg === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)

  try {
    const error: Error & { code?: string } = new Error(UTF8ToString(msg))
    if (code !== emnapi.NULL) {
      error.code = UTF8ToString(code)
    }
    envObject.tryCatch.setError(error)
  } catch (err) {
    envObject.tryCatch.setError(err)
    return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  }
  return emnapi.napi_clear_last_error(env)
}

function napi_throw_type_error (env: napi_env, code: const_char_p, msg: const_char_p): emnapi.napi_status {
  if (!emnapi.checkEnv(env)) return emnapi.napi_status.napi_invalid_arg
  const envObject = emnapi.envStore.get(env)!
  if (envObject.tryCatch.hasCaught()) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  emnapi.napi_clear_last_error(env)
  if (msg === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)

  try {
    const error: TypeError & { code?: string } = new TypeError(UTF8ToString(msg))
    if (code !== emnapi.NULL) {
      error.code = UTF8ToString(code)
    }
    envObject.tryCatch.setError(error)
  } catch (err) {
    envObject.tryCatch.setError(err)
    return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  }
  return emnapi.napi_clear_last_error(env)
}

function napi_throw_range_error (env: napi_env, code: const_char_p, msg: const_char_p): emnapi.napi_status {
  if (!emnapi.checkEnv(env)) return emnapi.napi_status.napi_invalid_arg
  const envObject = emnapi.envStore.get(env)!
  if (envObject.tryCatch.hasCaught()) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  emnapi.napi_clear_last_error(env)
  if (msg === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)

  try {
    const error: RangeError & { code?: string } = new RangeError(UTF8ToString(msg))
    if (code !== emnapi.NULL) {
      error.code = UTF8ToString(code)
    }
    envObject.tryCatch.setError(error)
  } catch (err) {
    envObject.tryCatch.setError(err)
    return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  }
  return emnapi.napi_clear_last_error(env)
}

function napi_is_error (env: napi_env, value: napi_value, result: Pointer<bool>): emnapi.napi_status {
  if (!emnapi.checkEnv(env)) return emnapi.napi_status.napi_invalid_arg
  if (value === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)
  if (result === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)
  const envObject = emnapi.envStore.get(env)!
  let r: boolean
  try {
    r = envObject.handleStore.get(value)!.value instanceof Error
  } catch (err) {
    envObject.tryCatch.setError(err)
    return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  }
  HEAPU8[result] = r ? 1 : 0
  return emnapi.napi_clear_last_error(env)
}

function napi_is_exception_pending (env: napi_env, result: Pointer<bool>): emnapi.napi_status {
  if (!emnapi.checkEnv(env)) return emnapi.napi_status.napi_invalid_arg
  if (result === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)
  const envObject = emnapi.envStore.get(env)!
  let r: boolean
  try {
    r = envObject.tryCatch.hasCaught()
  } catch (err) {
    envObject.tryCatch.setError(err)
    return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  }
  HEAPU8[result] = r ? 1 : 0
  return emnapi.napi_clear_last_error(env)
}

function napi_create_error (env: napi_env, code: napi_value, msg: napi_value, result: Pointer<napi_value>): emnapi.napi_status {
  if (!emnapi.checkEnv(env)) return emnapi.napi_status.napi_invalid_arg
  if (msg === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)
  if (result === emnapi.NULL) return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_invalid_arg)
  const envObject = emnapi.envStore.get(env)!

  let error: Error & { code?: string }
  try {
    const msgValue = envObject.handleStore.get(msg)!.value
    if (typeof msgValue !== 'string') {
      return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_string_expected)
    }

    error = new Error(msgValue)
    if (code !== emnapi.NULL) {
      error.code = envObject.handleStore.get(code)!.value
    }
  } catch (err) {
    envObject.tryCatch.setError(err)
    return emnapi.napi_set_last_error(env, emnapi.napi_status.napi_pending_exception)
  }
  HEAP32[result >> 2] = envObject.getCurrentScope().add(error).id
  return emnapi.napi_clear_last_error(env)
}

emnapiImplement('napi_get_last_error_info', napi_get_last_error_info)
emnapiImplement('napi_throw', napi_throw)
emnapiImplement('napi_throw_error', napi_throw_error)
emnapiImplement('napi_throw_type_error', napi_throw_type_error)
emnapiImplement('napi_throw_range_error', napi_throw_range_error)
emnapiImplement('napi_is_error', napi_is_error)
emnapiImplement('napi_create_error', napi_create_error)
emnapiImplement('napi_is_exception_pending', napi_is_exception_pending)
