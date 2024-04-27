{
  'targets': [
    {
      'target_name': 'ki_tests',
      'include_dirs': [ '<!@(node -p "require(\'..\').include_dir")' ],
      # C++17 is required for this feature:
      # https://stackoverflow.com/questions/8452952/c-linker-error-with-class-static-constexpr
      'cflags_cc': [ '-std=c++17' ],
      'xcode_settings': { 'OTHER_CFLAGS': [ '-std=c++17' ] },
      'msvs_settings': {
        'VCCLCompilerTool': {
          'AdditionalOptions': [ '/std:c++17' ],
        },
      },
      'defines': [
        'NAPI_VERSION=9',
      ],
      'sources': [
        'main.cc',
        'callback_tests.cc',
        'persistent_tests.cc',
        'property_tests.cc',
        'prototype_tests.cc',
        'types_tests.cc',
        'wrap_method_tests.cc',
      ],
    }
  ]
}
