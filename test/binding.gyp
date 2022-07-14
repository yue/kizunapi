{
  'targets': [
    {
      'target_name': 'ki_tests',
      'include_dirs': [
        '<!@(node -p "require(\'..\').include_dir")'
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
