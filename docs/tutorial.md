# Tutorial

This tutorial walks through the C++ APIs of kizunapi. On how to setup a native
module with the library, please check [README](../README.md).

There is currently no API reference.

## Basic type conversions

To convert types between C++ and JavaScript, you can use the `ki::ToNodeValue` and
`ki::FromNode` helpers:

```c++
napi_value value = ki::ToNodeValue(env, 8964);

int integer;
bool success = ki::FromNode(env, value, &integer);
```

Compilation errors will happen if unsupported types are passed.

There are also helpers to read and write properties of objects:

```c++
napi_value exports = ki::CreateObject(env);
ki::Set(env, exports,
        "str", "This is a string",
        "number", 19890604);

std::string str;
int number;
bool success = ki::Get(env, "str", &str, "number", &number);
```

## Functions

You can also convert `std::function` from/to JavaScript functions, the return
value and arguments will be converted automatically:

```c++
std::function<std::string()> func = []() { return std::string("str"); };
napi_value value = ki::ToNodeValue(env, func);

std::function<int(int, int)> add;
bool success = ki::FromNode(env, &add);
```

Function pointers work too:

```c++
int Add(int a, int b) { return a + b; }

napi_value add = ki::ToNodeValue(env, &Add);
```

When passing member functions, the converted JavaScript function will use the
`this` object as the `this` pointer when getting called. This is useful when
populating the prototype of a class:

```c++
struct Object {
  void Method() {}
};

ki::Set(env, prototype, "method", &Object::Method);
```

### Arguments

If you want to support multiple arguments from JavaScript, you can add
`ki::Arguments*` to the C++ function's parameters:

```c++
void CreateFile(const std::string& path, ki::Arguments* args) {
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
is a specialization of a `ki::Type<T>` template class:

```c++
struct Point {
  int x;
  int y;
};

namespace ki {

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
  static std::optional<Point> FromNode(napi_env env, napi_value value) {
    Point out;
    if (Get(env, value, "x", &out.x, "y", &out.y))
      return out;
    return std::nullopt;
  }
};

}  // namespace ki

Point p = {89, 64};
napi_value object = ki::ToNodeValue(env, p);
```

It is OK to ignore `ToNode` or `FromNode` method when the type only supports
one way conversion. The `name` property will be used to prompt type errors when
conversion failure happens in function invocations, so it should be the name of
JavaScript type instead of the C++ type.

## Classes

Mapping a C++ class to JavaScript is complicated, it involves lifetime
management, pointer safety, inheritance and lots of things. Most details have
been hidden by kizunapi, but you still need to understand the concepts to use
the APIs safely.

### Converting a class to JavaScript

Once you have defined a `ki::Type<T>` for a class, you can convert it to
JavaScript with the `ki:Class` helper:

```c++
class SimpleClass {};

namespace ki {

template<>
struct Type<SimpleClass> {
  static constexpr const char* name = "SimpleClass";
};

}  // namespace ki

ki::Set(env, exports, "SimpleClass", ki::Class<SimpleClass>());
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

The `ki::Type<T>::Constructor` accepts arbitrary arguments:

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
done by defining a `ki::Type<T>::Define` method:

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

Pointers to member functions are automatically recognized by kizunapi and you
usually just populate the `prototype` with methods you want to expose:

```c++
    ki::Set(env, prototype,
            "open", &SimpleClass::Open,
            "close", &SimpleClass::Close);
```

### Properties

You can also define properties by using the `ki::DefineProperties` API with
`ki::Property` helper:

```c++
int number = 19890604;

int Getter() {
  return number;
}

void Setter(int n) {
  number = n + 1;
}

ki::DefineProperties(env, exports,
                     ki::Property("number", ki::Getter(&Getter),
                                            ki::Setter(&Setter)));
```

The `ki::Getter` and `ki::Setter` can be omitted:

```c++
ki::DefineProperties(env, exports,
                     ki::Property("numberGetter", ki::Getter(&Getter)),
                     ki::Property("numberSetter", ki::Setter(&Setter)));
```

To set a value:

```c++
ki::Property("value", ki::ToNodeValue(env, "value"));
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

namespace ki {

template<>
struct Type<Date> {
  ...
  static void Define(napi_env env,
                     napi_value constructor,
                     napi_value prototype) {
    ki::DefineProperties(env, prototype,
                         ki::Property("year", &Date::year),
                         ki::Property("month", &Date::month),
                         ki::Property("day", &Date::day));
  }
};

}  // namespace ki
```

You can also pass the `napi_property_attributes` to set attributes, but please
note that the `napi_static` is not supported:

```c++
ki::Property("date", napi_writable | napi_enumerable, ki::ToNodeValue(env, 8964));
```

### Inheritance

By specifying `ki::Type<T>::Base`, you can hint the inheritance relationship to
kizunapi and the generated JavaScript classes will do prototype inheritance
automatically:

```c++
template<>
struct Type<Parent> {
  ...
};

template<>
struct Type<Child> {
  using Base = Parent;
  ...
};
```

### Passing pointers around

After creating a class in JavaScript, you can get the instance of it in C++:

```c++
SimpleClass* instance;
bool success = ki::FromNode(env, &instance);
```

But converting an C++ instance to JavaScript will fail with compilation error:

```c++
ki::ToNodeValue(env, new SimpleClass());  // does not compile
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

namespace ki {

template<>
struct Type<RefCounted> {
  static constexpr const char* name = "RefCounted";
  static inline RefCounted* Constructor() {
    return new RefCounted();
  }
};

template<>
struct TypeBridge<RefCounted> {
  static inline RefCounted* Wrap(RefCounted* ptr) {
    ptr->AddRef();
    return ptr;
  }
  static inline void Finalize(RefCounted* ptr) {
    ptr->Release();
  }
};

}  // namespace ki
```

The `ki::TypeBridge<T>::Wrap` is called when a C++ instance is being converted
to JavaScript, and it should return a pointer that will be stored in the
JavaScript object's internal field, which in most cases should just be the
pointer to the instance. And `ki::TypeBridge<T>::Finalize` is called with the
stored pointer when the JavaScript object is garbage collected. For a ref
counted class, `Wrap` and `Finalize` should be where you increase and decrease
ref counts.

The difference between `Destructor` and `Finalize` is, the `Destructor` is
called with the return value of `Constructor`, while the `Finalize` is called
with the return value of `Wrap`, if they are both defined then they will be both
called if a instance created by `new Class` is garbage collected. For most
cases, the `ki::Type<T>::Destructor` can be omitted if a `Finalize` method has
been defined.

Also note that if a `ki::TypeBridge<T>::Wrap` is defined, it will be called for
the pointer returned by `Constructor` automatically.

### Object internal storage and unwrapping

For JavaScript objects created by kizunapi for wrapping C++ instances, they all
have an internal storage to store a C++ pointer, which in most cases is just the
pointer of the C++ instance they are wrapping.

However it is possible that the internal storage stores the C++ instance in
another type, for example weak pointer, and in this case you must define a
`ki::TypeBridge<T>::Unwrap` to instruct how to receive the C++ instance from the
internal storage:

```c++
template<>
struct Type<Factory> {
  static constexpr const char* name = "Factory";
  static Factory* Constructor() {
    return new Factory;
  }
  static void Destructor(Factory* ptr) {
    delete ptr;
  }
};

template<>
struct TypeBridge<Factory> {
  static WeakPtr<Factory>* Wrap(Factory* ptr) {
    return new WeakPtr<Factory>(ptr->GetWeakPtr());
  }
  static Factory* Unwrap(WeakPtr<Factory>* data) {
    return data->Get();
  }
  static void Finalize(WeakPtr<Factory>* data) {
    delete data;
  }
};
```
