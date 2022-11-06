{
    'variables': {
      'OS': 'emscripten',
    },
    'target_defaults': {
        # 'shared_library' / 'loadable_module' does something different on Emscripten
        # and only leads to unsupported linker flags being passed to the linker.
        # 'executable' in JS+Wasm sense can act as a library we expect.
        'type': 'executable',
        # Set .mjs extension, both because emcc looks at the extension to know
        # what to generate, and because we do in fact want main output
        # to be an ECMAScript module.
        'product_extension': 'mjs',
        'ldflags': [
            '--js-library=<!(node -p "require(\'@tybys/emnapi\').js_library")',
            '-sNODEJS_CATCH_EXIT=0',
            '-sNODEJS_CATCH_REJECTION=0',
            '-sAUTO_JS_LIBRARIES=0',
            '-sAUTO_NATIVE_LIBRARIES=0',
            '--bind',
        ],
        'include_dirs': [
            '<!(node -p "require(\'@tybys/emnapi\').include_dir")',
        ],
        'sources': [
            '<!@(node -p "require(\'@tybys/emnapi\').sources.map(x => JSON.stringify(path.relative(process.cwd(), x))).join(\' \')")'
        ]
    }
}
