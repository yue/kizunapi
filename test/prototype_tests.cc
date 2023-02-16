// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <kizunapi.h>

namespace {

class SimpleClass {
};

class ClassWithConstructor {
};

class ThrowInConstructor {
 public:
  explicit ThrowInConstructor(napi_env env) {
    napi_throw_error(env, nullptr, "Throwed in constructor");
  }
};

class RefCounted {
 public:
  RefCounted() : count_(0) {}

  RefCounted& operator=(const RefCounted&) = delete;
  RefCounted(const RefCounted&) = delete;

  void AddRef() {
    count_++;
  }

  void Release() {
    if (--count_ == 0)
      delete this;
  }

  int Count() const {
    return count_;
  }

  void* data = nullptr;

 private:
  ~RefCounted() = default;

  int count_;
};

class Parent {
 public:
  int ParentMethod() {
    return 89;
  }
};

class Child : public Parent {
 public:
  int ChildMethod() {
    return 64;
  }
};

template<typename T>
class WeakPtr {
 public:
  explicit WeakPtr(RefCounted* ref) : ref_(ref) {
    ref_->AddRef();
  }

  WeakPtr(const WeakPtr&) = delete;

  WeakPtr(WeakPtr&& other) {
    ref_ = other.ref_;
    other.ref_ = nullptr;
  }

  ~WeakPtr() {
    if (ref_)
      ref_->Release();
  }

  T* Get() {
    return static_cast<T*>(ref_->data);
  }

 private:
  RefCounted* ref_;
};

class WeakFactory {
 public:
  WeakFactory() : ref_(new RefCounted) {
    ref_->data = this;
    ref_->AddRef();
  }

  ~WeakFactory() {
    ref_->data = nullptr;
    ref_->Release();
  }

  void Destroy() {
    delete this;
  }

  WeakPtr<WeakFactory> GetWeakPtr() {
    return WeakPtr<WeakFactory>(ref_);
  }

 private:
  RefCounted* ref_;
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

namespace ki {

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
struct TypeBridge<RefCounted> {
  static RefCounted* Wrap(RefCounted* ptr) {
    ptr->AddRef();
    return ptr;
  }
  static void Finalize(RefCounted* ptr) {
    ptr->Release();
  }
};

template<>
struct Type<RefCounted> {
  static constexpr const char* name = "RefCounted";
  static RefCounted* Constructor() {
    return new RefCounted;
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype, "count", &RefCounted::Count);
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
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype, "parentMethod", &Parent::ParentMethod);
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
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype, "childMethod", &Child::ChildMethod);
  }
};

template<>
struct TypeBridge<WeakFactory> {
  static WeakPtr<WeakFactory>* Wrap(WeakFactory* ptr) {
    return new WeakPtr<WeakFactory>(ptr->GetWeakPtr());
  }
  static WeakFactory* Unwrap(WeakPtr<WeakFactory>* data) {
    return data->Get();
  }
  static void Finalize(WeakPtr<WeakFactory>* data) {
    delete data;
  }
};

template<>
struct Type<WeakFactory> {
  static constexpr const char* name = "WeakFactory";
  static WeakFactory* Constructor() {
    return new WeakFactory;
  }
  static void Destructor(WeakFactory* ptr) {
    delete ptr;
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype, "destroy", &WeakFactory::Destroy);
  }
};

}  // namespace ki

void run_prototype_tests(napi_env env, napi_value binding) {
  ki::Set(env, binding,
          "SimpleClass", ki::Class<SimpleClass>(),
          "ClassWithConstructor", ki::Class<ClassWithConstructor>(),
          "pointerOfClass", &PointerOf<ClassWithConstructor>,
          "ThrowInConstructor", ki::Class<ThrowInConstructor>());

  RefCounted* ref_counted = new RefCounted;
  ki::Set(env, binding,
          "refCounted", ref_counted,
          "RefCounted", ki::Class<RefCounted>(),
          "passThroughRefCounted", &PassThrough<RefCounted>);

  ki::Set(env, binding,
          "Child", ki::Class<Child>(),
          "Parent", ki::Class<Parent>(),
          "pointerOfParent", &PointerOf<Parent>,
          "pointerOfChild", &PointerOf<Child>);

  WeakFactory* factory = new WeakFactory;
  ki::Set(env, binding,
          "weakFactory", factory,
          "WeakFactory", ki::Class<WeakFactory>());
}
