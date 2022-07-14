# Tutorial

This tutorial walks through the C++ APIs of napi-bind. On how to setup
a native module with the library, please check [README](../README.md).

There is currently no API reference.

## Basic type conversions

To convert types between C++ and JavaScript, you can use the `nb::ToNode` and
`nb::FromNode` helpers:

```c++
napi_value value = nb::ToNode(env, 8964);

int integer;
bool success = nb::FromNode(env, value, &integer);
```

Compilation errors will happen if unsupported types are passed.

There are also helpers to read and write properties of objects:

```c++
napi_value exports = nb::CreateObject(env);
nb::Set(env, exports,
        "str", "This is a string",
        "number", 19890604);

std::string str;
int number;
bool success = nb::Get(env, "str", &str, "number", &number);
```

## Functions

You can also convert `std::function` from/to JavaScript functions, the return
value and arguments will be converted automatically:

```c++
std::function<std::string()> func = []() { return std::string("str"); };
napi_value value = nb::ToNode(env, func);

std::function<int(int, int)> add;
bool success = nb::FromNode(env, &add);
```

Function pointers work too:

```c++
int Add(int a, int b) { return a + b; }

napi_value add = nb::ToNode(env, &Add);
```

When passing member functions, the converted JavaScript function will use the
`this` object as the `this` pointer when getting called. This is useful when
populating the prototype of a class:

```c++
struct Object {
  void Method() {}
};

nb::Set(env, prototype, "method", &Object::Method);
```

### Arguments

If you want to support multiple arguments from JavaScript, you can add
`nb::Arguments*` to the C++ function's parameters:

```c++
void CreateFile(const std::string& path, nb::Arguments* args) {
  int options = 0;
  args->GetNext(&options);
  create_file(path, options);
}
```

You can also use `napi_value` and `napi_env` as parameters if you want to deal
with the JavaScript types directly.

```c++
void WriteToPasteboard(napi_env env, napi_value value) {
}
```

## Custom types

The [built rules](../src/types.h) only support conversions of a very limited
set of types, to convert a custom type, you need to define a custom rule, which
is a specialization of a `nb::Type<T>` template class:

```c++
struct Point {
  int x;
  int y;
};

namespace nb {

template<>
struct Type<Point> {
  static constexpr const char* name = "Point";
  static inline napi_status ToNode(napi_env env,
                                   const Point& value,
                                   napi_value* result) {
    napi_status s = napi_create_object(env, result);
    if (s != napi_ok)
      return s;
    if (!Set(env, *result, "x", value.x, "y", value.y))
      return napi_generic_failure;
    return napi_ok;
  }
  static napi_status FromNode(napi_env env, napi_value value, Point* out) {
    return Get(env, value, "x", &out->x, "y", &out->y) ? napi_ok
                                                       : napi_generic_failure;
  }
};

}  // namespace nb

Point p = {89, 64};
napi_value object = nb::ToNode(env, p);
```

It is OK to ignore `ToNode` or `FromNode` method when the type only supports
one way conversion. The `name` property will be used to prompt type errors when
conversion failure happens in function invocations, so it should be the name of
JavaScript type instead of the C++ type.

## Classes

Mapping a C++ class to JavaScript is complicated, it involves lifetime
management, pointer safety, inheritance and lots of things. Most details have
been hidden by napi-bind, but you still need to understand the concepts to use
the APIs safely.

### Converting a class to JavaScript

Once you have defined a `nb::Type<T>` for a class, you can convert it to
JavaScript with the `nb:Class` helper:

```c++
class SimpleClass {};

namespace nb {

template<>
struct Type<SimpleClass> {
  static constexpr const char* name = "SimpleClass";
};

}  // namespace nb

nb::Set(env, exports, "SimpleClass", nb::Class<SimpleClass>());
```

### Constructor and destructor

If you have tried the code above, you will find that calling `new SimpleClass`
in JavaScript will throw exceptions, this is because there is no constructor
defined.

In C++ a class may have multiple constructor overloads, while in JavaScript you
can only have one, so you must explicitly define how the constructor is called,
along with how the class is destructed:

```c++
template<>
struct Type<SimpleClass> {
  static constexpr const char* name = "SimpleClass";
  static inline SimpleClass* Constructor() {
    return new SimpleClass();
  }
  static inline void Destructor(SimpleClass* ptr) {
    delete ptr;
  }
};
```

The `nb::Type<T>::Constructor` accepts arbitrary arguments:

```c++
  static inline RandomNumberGenerator* Constructor(int seed) {
    return new RandomNumberGenerator(seed);
  }
```

And depending on what you are wrapping, you don't even have to call `new` and
`delete`:

```obj-c++
template<>
struct Type<NSWindow> {
  static constexpr const char* name = "NSWindow";
  static inline NSWindow* Constructor() {
    NSWindow* window = [[NSWindow alloc] init];
    [window setReleasedWhenClosed: NO];
    return window;
  }
  static inline void Destructor(NSWindow* ptr) {
    [ptr release];
  }
};
```

### Prototype

With constructor and destructor created for the class, the next thing you want
to do is usually filling its prototype with methods and properties. This can be
done by defining a `nb::Type<T>::Define` method:

```c++
template<>
struct Type<SimpleClass> {
  ...
  static void Define(napi_env env,
                     napi_value constructor,
                     napi_value prototype) {
  }
};
```

The `constructor` is the function object used for doing `new SimpleClass`, you
can add properties to it to implement static class methods and properties. And
the `prototype` is the prototype object, i.e. `SimpleClass.prototype`, it is
where you should add class methods and properties.

Pointers to member functions are automatically recognized by napi-bind and you
usually just populate the `prototype` with methods you want to expose:

```c++
    nb::Set(env, prototype,
            "open", &SimpleClass::Open,
            "close", &SimpleClass::Close);
```

### Properties

You can also define properties by using the `nb::DefineProperties` API with
`nb::Property` helper:

```c++
int number = 19890604;

int Getter() {
  return number;
}

void Setter(int n) {
  number = n + 1;
}

nb::DefineProperties(env, exports,
                     nb::Property("number", nb::Getter(&Getter),
                                            nb::Setter(&Setter)));
```

The `nb::Getter` and `nb::Setter` can be omitted:

```c++
nb::DefineProperties(env, exports,
                     nb::Property("numberGetter", nb::Getter(&Getter)),
                     nb::Property("numberSetter", nb::Setter(&Setter)));
```

To set a value:

```c++
nb::Property("value", nb::ToNode(env, "value"));
```

For member data of classes, you can pass pointers to them to set setter and
getter automatically:

```c++
class Date {
 public:
  int year = 1989;
  int month = 6;
  int day = 4;
};

namespace nb {

template<>
struct Type<Date> {
  ...
  static void Define(napi_env env,
                     napi_value constructor,
                     napi_value prototype) {
    nb::DefineProperties(env, prototype,
                         nb::Property("year", &Date::year),
                         nb::Property("month", &Date::month),
                         nb::Property("day", &Date::day));
  }
};

}  // namespace nb
```

You can also pass the `napi_property_attributes` to set attributes, but please
note that the `napi_static` is not supported:

```c++
nb::Property("date", napi_writable | napi_enumerable, nb::ToNode(env, 8964));
```

### Inheritance

By specifying `nb::Type<T>::base`, you can hint the inheritance relationship to
napi-bind and the generated JavaScript classes will do prototype inheritance
automatically:

```c++
template<>
struct Type<Parent> {
  ...
};

template<>
struct Type<Child> {
  using base = Parent;
  ...
};
```

### Passing pointers around

After creating a class in JavaScript, you can get the instance of it in C++:

```c++
SimpleClass* instance;
bool success = nb::FromNode(env, &instance);
```

But converting an C++ instance to JavaScript will fail with compilation error:

```c++
nb::ToNode(env, new SimpleClass());  // does not compile
```

This is because the latter involves lifetime management of the C++ instances.

If an instance is only created and destroyed by JavaScript, then its lifetime is
totally managed by the JavaScript runtime. However if we allow passing C++
instances to JavaScript, then it could happen that an instance is created by
C++ but both managed by JavaScript and C++, and crashes like double frees and
wild pointers will happen easily. For most cases this is solved by using ref
counting to manage the instances.

To enabling passing C++ instances to JavaScript, you must explicitly define how
the instance's lifetime is managed:

```c++
class RefCounted {
 public:
  RefCounted() : count_(0) {}
  RefCounted& operator=(const RefCounted&) = delete;
  RefCounted(const RefCounted&) = delete;

  void AddRef() { ++count_; }
  void Release() {
    if (--count_ == 0)
      delete this;
  }

 private:
  ~RefCounted() = default;

  int count_;
};

namespace nb {

template<>
struct Type<RefCounted> {
  static constexpr const char* name = "RefCounted";
  static inline RefCounted* Wrap(RefCounted* ptr) {
    ptr->AddRef();
    return ptr;
  }
  static inline void Finalize(RefCounted* ptr) {
    ptr->Release();
  }
  static inline RefCounted* Constructor() {
    return new RefCounted();
  }
};

}  // namespace nb
```

The `nb::Type<T>::Wrap` is called when a C++ instance is being converted to
JavaScript, and it should return a pointer that will be stored in the JavaScript
object's internal field, which in most cases should just be the pointer to the
instance. And `nb::Type<T>::Finalize` is called with the stored pointer when the
JavaScript object is garbage collected. For a ref counted class, `Wrap` and
`Finalize` should be where you increase and decrease ref counts.

The difference between `Destructor` and `Finalize` is, the `Destructor` is
called with the return value of `Constructor`, while the `Finalize` is called
with the return value of `Wrap`, if they are both defined then they will be both
called if a instance created by `new Class` is garbage collected. For most
cases, the `nb::Type<T>::Destructor` can be omitted if a `Finalize` method has
been defined.

Also note that if a `nb::Type<T>::Wrap` is defined, it will be called for the
pointer returned by `Constructor` automatically.

### Object internal storage and unwrapping

For JavaScript objects created by napi-bind for wrapping C++ instances, they all
have an internal storage to store a C++ pointer, which in most cases is just the
pointer of the C++ instance they are wrapping.

However it is possible that the internal storage stores the C++ instance in
another type, for example weak pointer, and in this case you must define a
`nb::Type<T>::Unwrap` to instruct how to receive the C++ instance from the
internal storage:

```c++
template<>
struct Type<Factory> {
  static constexpr const char* name = "Factory";
  static WeakPtr<Factory>* Wrap(Factory* ptr) {
    return new WeakPtr<Factory>(ptr->GetWeakPtr());
  }
  static Factory* Unwrap(WeakPtr<Factory>* data) {
    return data->Get();
  }
  static void Finalize(WeakPtr<Factory>* data) {
    delete data;
  }
  static Factory* Constructor() {
    return new Factory;
  }
  static void Destructor(Factory* ptr) {
    delete ptr;
  }
};
```
