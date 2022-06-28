{
  'targets': [
    {
      'target_name': 'nb_tests',
      'include_dirs': [
        '<!@(node -p "require(\'..\').include")'
      ],
      'sources': [
        'main.cc',
      ],
    }
  ]
}
