function napi_set_instance_data (env: napi_env, data: void_p, finalize_cb: napi_finalize, finalize_hint: void_p): napi_status {
  $CHECK_ENV!(env)
  const envObject = emnapiCtx.envStore.get(env)!
  $from64('data')
  $from64('finalize_cb')
  $from64('finalize_hint')
  envObject.setInstanceData(data, finalize_cb, finalize_hint)
  return envObject.clearLastError()
}

function napi_get_instance_data (env: napi_env, data: void_pp): napi_status {
  $CHECK_ENV!(env)
  const envObject = emnapiCtx.envStore.get(env)!
  $CHECK_ARG!(envObject, data)
  $from64('data')

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  const value = envObject.getInstanceData()
  $makeSetValue('data', 0, 'value', '*')
  return envObject.clearLastError()
}

emnapiImplement('napi_set_instance_data', 'ipppp', napi_set_instance_data)
emnapiImplement('napi_get_instance_data', 'ipp', napi_get_instance_data)
