{
    # 'include_dirs': [
    #     '<!(node -p "require(\'@tybys/emnapi\').include_dir")',
    # ],
    'libraries': [
        '--js-library=<!(node -p "require(\'@tybys/emnapi\').js_library")'
    ],
    'defines': [
        'NAPI_DISABLE_CPP_EXCEPTIONS',
        'NODE_ADDON_API_ENABLE_MAYBE'
    ],
    'sources': '<!(node -p "require(\'@tybys/emnapi\').sources")'
}
