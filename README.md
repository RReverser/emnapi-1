# emnapi

[WIP] 尝试用 JavaScript 为 emscripten 实现 [Node-API](https://nodejs.org/dist/latest-v14.x/docs/api/n-api.html) (v14.16.0)

## 构建

设置 `$EMSDK` 环境变量为 emsdk 根目录，并确保 Emscripten 工具链二进制目录（`$EMSDK/upstream/emscripten`）和 CMake 在 `$PATH` 里

未安装 `make` 的 Windows 用户请使用 Visual Studio Developer Command Prompt 跑命令（需要用到 `nmake`）

```bash
npm install -g @tybys/cgen
npm install
npm run rebuild

# test
npm test
```

## 使用

仅支持运行在近期版本的现代浏览器和 Node.js LTS 版本，不支持 IE

Emscripten 需要 v2.0.2 以上的版本，链接上一步构建出来的 js 库，需要添加 `-sEXPORTED_FUNCTIONS=['_malloc','_free']`

```sh
emcc -O3 -I./include --js-library=./dist/library_napi.js \
     -sALLOW_MEMORY_GROWTH=1 \
     -sEXPORTED_FUNCTIONS=['_malloc','_free'] \
     -o hello.js \
     hello.c
```

```c
// hello.c
#include <node_api.h>
#include <string.h>

#define GET_AND_THROW_LAST_ERROR(env)                                    \
  do {                                                                   \
    const napi_extended_error_info *error_info;                          \
    napi_get_last_error_info((env), &error_info);                        \
    bool is_pending;                                                     \
    napi_is_exception_pending((env), &is_pending);                       \
    if (!is_pending) {                                                   \
      const char* error_message = error_info->error_message != NULL ?    \
        error_info->error_message :                                      \
        "empty error message";                                           \
      napi_throw_error((env), NULL, error_message);                      \
    }                                                                    \
  } while (0)

#define NAPI_CALL_BASE(env, the_call, ret_val)                           \
  do {                                                                   \
    if ((the_call) != napi_ok) {                                         \
      GET_AND_THROW_LAST_ERROR((env));                                   \
      return ret_val;                                                    \
    }                                                                    \
  } while (0)

#define NAPI_CALL(env, the_call)                                         \
  NAPI_CALL_BASE(env, the_call, NULL)

#define DECLARE_NAPI_PROPERTY(name, func)                                \
  { (name), NULL, (func), NULL, NULL, NULL, napi_default, NULL }

static napi_value Method(napi_env env, napi_callback_info info) {
  napi_value world;
  const char* str = "world";
  size_t str_len = strlen(str);
  NAPI_CALL(env, napi_create_string_utf8(env, str, str_len, &world));
  return world;
}

NAPI_MODULE_INIT() {
  napi_property_descriptor desc = DECLARE_NAPI_PROPERTY("hello", Method);
  NAPI_CALL(env, napi_define_properties(env, exports, 1, &desc));
  return exports;
}
```

导出对象默认是 `Module.emnapiExports`，可通过在包含 `node_api.h` 前定义 `NODE_GYP_MODULE_NAME` 宏来设置导出的 key 值。

```html
<script>
var Module = {
  onRuntimeInitialized: function () {
    var binding = Module.emnapiExports;
    console.log(binding.hello()); // output 'world'
  }
};
</script>

<script src="hello.js"></script>
```

## 已实现

进度：66 / 115 \[57%\]

斜体加粗表示该 API 受限于 JavaScript 运行时能力，可能与原生行为不一致，或是其残废的简易实现，不推荐使用。

中划线表示该 API 不可实现，调用时永远返回 `napi_generic_failure` 状态。

- [x] napi_get_last_error_info
- [x] napi_get_undefined
- [x] napi_get_null
- [x] napi_get_global
- [x] napi_get_boolean
- [x] napi_create_object
- [x] napi_create_array
- [x] napi_create_array_with_length
- [x] napi_create_double
- [x] napi_create_int32
- [x] napi_create_uint32
- [x] napi_create_int64
- [x] napi_create_string_latin1
- [x] napi_create_string_utf8
- [x] napi_create_string_utf16
- [x] napi_create_symbol
- [x] napi_create_function
- [x] napi_create_error
- [x] napi_create_type_error
- [x] napi_create_range_error
- [x] napi_typeof
- [x] napi_get_value_double
- [x] napi_get_value_int32
- [x] napi_get_value_uint32
- [x] napi_get_value_int64
- [x] napi_get_value_bool
- [x] napi_get_value_string_latin1
- [x] napi_get_value_string_utf8
- [x] napi_get_value_string_utf16
- [ ] napi_coerce_to_bool
- [ ] napi_coerce_to_number
- [ ] napi_coerce_to_object
- [ ] napi_coerce_to_string
- [ ] napi_get_prototype
- [ ] napi_get_property_names
- [ ] napi_set_property
- [ ] napi_has_property
- [ ] napi_get_property
- [ ] napi_delete_property
- [ ] napi_has_own_property
- [x] napi_set_named_property
- [ ] napi_has_named_property
- [ ] napi_get_named_property
- [ ] napi_set_element
- [ ] napi_has_element
- [ ] napi_get_element
- [ ] napi_delete_element
- [x] napi_define_properties
- [ ] napi_is_array
- [ ] napi_get_array_length
- [ ] napi_strict_equals
- [x] napi_call_function
- [ ] napi_new_instance
- [ ] napi_instanceof
- [x] napi_get_cb_info
- [ ] napi_get_new_target
- [ ] napi_define_class
- [x] ***napi_wrap*** (require `FinalizationRegistry`)
- [x] ***napi_unwrap*** (require `FinalizationRegistry`)
- [x] ***napi_remove_wrap*** (require `FinalizationRegistry`)
- [x] ***napi_create_external*** (require `FinalizationRegistry`)
- [x] ***napi_get_value_external*** (require `FinalizationRegistry`)
- [x] ***napi_create_reference*** (require `FinalizationRegistry`)
- [x] ***napi_delete_reference*** (require `FinalizationRegistry`)
- [x] ***napi_reference_ref*** (require `FinalizationRegistry`)
- [x] ***napi_reference_unref*** (require `FinalizationRegistry`)
- [x] ***napi_get_reference_value*** (require `FinalizationRegistry`)
- [x] napi_open_handle_scope
- [x] napi_close_handle_scope
- [x] napi_open_escapable_handle_scope
- [x] napi_close_escapable_handle_scope
- [x] napi_escape_handle
- [x] napi_throw
- [x] napi_throw_error
- [x] napi_throw_type_error
- [x] napi_throw_range_error
- [x] napi_is_error
- [x] napi_is_exception_pending
- [x] napi_get_and_clear_last_exception
- [ ] napi_is_arraybuffer
- [x] ***napi_create_arraybuffer*** (`data` is always NULL)
- [x] ~~napi_create_external_arraybuffer~~
- [ ] ***napi_get_arraybuffer_info*** (`data` is always NULL)
- [ ] napi_is_typedarray
- [x] napi_create_typedarray
- [ ] napi_get_typedarray_info
- [x] napi_create_dataview
- [ ] napi_is_dataview
- [ ] napi_get_dataview_info
- [x] napi_get_version
- [ ] napi_create_promise
- [ ] napi_resolve_deferred
- [ ] napi_reject_deferred
- [ ] napi_is_promise
- [x] napi_run_script
- [x] ~~napi_adjust_external_memory~~
- [x] napi_create_date
- [ ] napi_is_date
- [ ] napi_get_date_value
- [x] ***napi_add_finalizer*** (require `FinalizationRegistry`)
- [ ] napi_create_bigint_int64
- [ ] napi_create_bigint_uint64
- [ ] napi_create_bigint_words
- [ ] napi_get_value_bigint_int64
- [ ] napi_get_value_bigint_uint64
- [ ] napi_get_value_bigint_words
- [ ] napi_get_all_property_names
- [x] napi_set_instance_data
- [x] napi_get_instance_data
- [ ] napi_detach_arraybuffer
- [ ] napi_is_detached_arraybuffer
- [ ] napi_type_tag_object
- [ ] napi_check_object_type_tag
- [ ] napi_object_freeze
- [ ] napi_object_seal
