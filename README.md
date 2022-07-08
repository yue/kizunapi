# napi-bind

A set of C++ classes for type convertion between C++ and JavaScript using
[Node-API](https://nodejs.org/api/n-api.html).

Unlike most other libraries that mainly focus on simplifying the use of Node-API
in C++, this library provides high-level APIs to convert functions and classes
with minimum hand-written code.

__This project is still under heavy development so use it at your own risk.__

## Usage

1. Add this module to dependency in `package.json`:

```json
  "dependencies": {
    "napi-bind": "*",
  }
```

2. Add include directory to `binding.gyp`:

```python
  'include_dirs': ["<!(node -p \"require('napi-bind').include_dir\")"],
```

3. In source code:

```c
#include <nbind.h>
```

## Tutorial

The APIs of napi-bind are under the `nb` namespace, function and class names
are usually very short so it is recommended to not use `using namespace nb`.

The core principle of napi-bind is, you don't write wrapper classes, instead
you write type descriptions and the library will do the rest of the work.

### Type convertion for basic types

### Functions

### Lifetime of classes

### Type convertion for classes

### Caching in properties and methods

## Contributing

For new features, it is recommended to start an issue first before creating a
pull request.

Generally I would encourage forking if you would like to add a feature that
needs over a thousand lines, because I don't really have much time maintaining
this project. But bug reports are very welcomed.
