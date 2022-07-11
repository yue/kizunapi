# napi-bind

A set of C++ classes for type convertion between C++ and JavaScript using
[Node-API](https://nodejs.org/api/n-api.html).

Unlike most other binding libraries which focus on simplifying the use of
Node-API in C++, napi-bind provides high-level APIs to convert functions and
classes with minimum hand-written code.

__This project is still under heavy development.__

## Usage

1. Add this module to dependencies in `package.json`:

```json
  "dependencies": {
    "napi-bind": "*",
  }
```

2. Add include directory in `binding.gyp`:

```python
  'include_dirs': ["<!(node -p \"require('napi-bind').include_dir\")"],
```

3. In source code:

```c
#include <nbind.h>
```

## Docs

* [Tutorial](docs/tutorial.md)
* API Reference

## Example

This example maps C++ classes with inheritance relationship to JavaScript
using non-intrusive APIs.

```c
#include <nbind.h>

class Parent {
 public:
  int Year() const {
    return 1989;
  }
};

class Child : public Parent {
 public:
  Child(int month, day) : month_(month), day_(day) {}

  std::string Date() const {
    return std::to_string(month_) + std::to_string(day_);
  }

 private:
  int month_;
  int day_;
};

namespace nb {

template<>
struct Type<Parent> {
  static constexpr const char* name = "Parent";
  static Parent* Constructor() {
    return new Parent();
  }
  static void Destructor(Parent* ptr) {
    delete ptr;
  }
  static void Define(napi_env env,
                     napi_value constructor,
                     napi_value prototype) {
    Set(env, prototype, "year", &Parent::Year);
  }
};

template<>
struct Type<Child> {
  using base = Parent;
  static constexpr const char* name = "Child";
  static Child* Constructor(int month, int day) {
    return new Child(month, day);
  }
  static void Destructor(Child* ptr) {
    delete ptr;
  }
  static void Define(napi_env env,
                     napi_value constructor,
                     napi_value prototype) {
    Set(env, prototype, "date", &Child::Date);
  }
};

}  // namespace nb

napi_value Init(napi_env env, napi_value exports) {
  nb::Set(env, exports,
          "Parent", nb::Constructor<Parent>(),
          "Child", nb::Constructor<Child>());
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
```

## Contributing

For new features, it is recommended to start an issue first before creating a
pull request.

Generally I would encourage forking if you would like to add a feature that
needs over a thousand lines, because I don't really have much time maintaining
this project. But bug reports are very welcomed.
