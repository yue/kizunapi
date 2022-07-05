#include <memory>

#include <nbind.h>

namespace {

class SimpleClass {
};

class ClassWithConstructor {
};

class ThrowInConstructor {
 public:
  ThrowInConstructor(napi_env env) {
    napi_throw_error(env, nullptr, "Throwed in constructor");
  }
};

class RefCounted {
 public:
  RefCounted() : count_(1) {}

  RefCounted& operator=(const RefCounted&) = delete;
  RefCounted(const RefCounted&) = delete;

  void AddRef() {
    count_++;
  }

  void Release() {
    if (--count_ == 0)
      delete this;
  }

  int count() const { return count_; }

 private:
  ~RefCounted() = default;

  int count_;
};

class Parent {
};

class Child : public Parent {
};

template<typename T>
int64_t PointerOf(T* ptr) {
  return reinterpret_cast<int64_t>(ptr);;
}

template<typename T>
T* PassThrough(T* ptr) {
  return ptr;
}

}  // namespace

namespace nb {

template<>
struct Type<SimpleClass> {
  static constexpr const char* name = "SimpleClass";
};

template<>
struct Type<ClassWithConstructor> {
  static constexpr const char* name = "ClassWithConstructor";
  static ClassWithConstructor* Constructor() {
    return new ClassWithConstructor();
  }
  static void Destructor(ClassWithConstructor* ptr) {
    delete ptr;
  }
};

template<>
struct Type<ThrowInConstructor> {
  static constexpr const char* name = "ThrowInConstructor";
  static ThrowInConstructor* Constructor(napi_env env) {
    return new ThrowInConstructor(env);
  }
  static void Destructor(ThrowInConstructor* ptr) {
    delete ptr;
  }
};

template<>
struct Type<RefCounted> {
  static constexpr const char* name = "RefCounted";
  static void* Wrap(RefCounted* ptr) {
    ptr->AddRef();
    return ptr;
  }
  static void Finalize(void* ptr) {
    static_cast<RefCounted*>(ptr)->Release();
  }
};

template<>
struct Type<Parent> {
  static constexpr const char* name = "Parent";
  static Parent* Constructor() {
    return new Parent();
  }
  static void Destructor(Parent* ptr) {
    delete ptr;
  }
};

template<>
struct Type<Child> {
  using base = Parent;
  static constexpr const char* name = "Child";
  static Child* Constructor() {
    return new Child();
  }
  static void Destructor(Child* ptr) {
    delete ptr;
  }
};

}  // namespace nb

void run_prototype_tests(napi_env env, napi_value binding) {
  nb::Set(env, binding,
          "SimpleClass", nb::Constructor<SimpleClass>(),
          "ClassWithConstructor", nb::Constructor<ClassWithConstructor>(),
          "pointerOfClass", &PointerOf<ClassWithConstructor>,
          "ThrowInConstructor", nb::Constructor<ThrowInConstructor>());

  RefCounted* ref_counted = new RefCounted;
  nb::Set(env, binding,
          "refCounted", ref_counted,
          "RefCounted", nb::Constructor<RefCounted>(),
          "passThroughRefCounted", &PassThrough<RefCounted>);

  nb::Set(env, binding,
          "Child", nb::Constructor<Child>(),
          "Parent", nb::Constructor<Parent>(),
          "pointerOfParent", &PointerOf<Parent>,
          "pointerOfChild", &PointerOf<Child>);
}
