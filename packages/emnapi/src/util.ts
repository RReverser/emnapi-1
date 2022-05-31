// eslint-disable-next-line @typescript-eslint/no-unused-vars
function emnapiImplement (name: string, compilerTimeFunction: Function, deps?: string[]): void {
  mergeInto(LibraryManager.library, {
    [name]: compilerTimeFunction,
    [name + '__deps']: (['$emnapi', '$emnapiInit']).concat(deps ?? [])
  })
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
// declare function napi_clear_last_error (env: napi_env): napi_status
// eslint-disable-next-line @typescript-eslint/no-unused-vars
// declare function napi_set_last_error (env: napi_env, error_code: napi_status, engine_error_code: uint32_t, engine_reserved: void_p): napi_status

// mergeInto(LibraryManager.library, {
//   $napi_set_last_error: function (env: napi_env, error_code: napi_status, engine_error_code: uint32_t, engine_reserved: void_p) {
//     const p = env >> 2
//     Module.HEAP32[p + 3] = error_code
//     Module.HEAPU32[p + 2] = engine_error_code
//     Module.HEAP32[p + 1] = engine_reserved
//     return error_code
//   },

//   $napi_clear_last_error: function (env: napi_env) {
//     const p = env >> 2
//     Module.HEAP32[p + 3] = napi_status.napi_ok
//     Module.HEAPU32[p + 2] = 0
//     Module.HEAP32[p + 1] = NULL
//     Module.HEAP32[p] = NULL
//     return napi_status.napi_ok
//   }
// })
