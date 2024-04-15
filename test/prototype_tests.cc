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

 protected:
  ~RefCounted() = default;

 private:
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

Parent* ChildToParent(Child* child) {
  return child;
}

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

class Copiable {
 public:
  static int count_;

  static int Count() { return count_; }

  Copiable() { count_++; }
  Copiable(const Copiable&) { count_++; }
  Copiable(Copiable&&) { count_++; }

  ~Copiable() { count_--; }
};

// static
int Copiable::count_ = 0;

template<typename T>
int64_t PointerOf(T* ptr) {
  return reinterpret_cast<int64_t>(ptr);;
}

template<typename T>
T PassThrough(T val) {
  return val;
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
struct Type<RefCounted> {
  static constexpr const char* name = "RefCounted";
  static RefCounted* Constructor() {
    return new RefCounted;
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype, "count", &RefCounted::Count);
  }
};

template<typename T>
struct TypeBridge<T, typename std::enable_if<std::is_base_of<
                         RefCounted, T>::value>::type> {
  static T* Wrap(T* ptr) {
    ptr->AddRef();
    return ptr;
  }
  static void Finalize(T* ptr) {
    ptr->Release();
  }
};

template<>
struct Type<Parent> {
  static constexpr const char* name = "Parent";
  static Parent* Constructor() {
    return new Parent();
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype, "parentMethod", &Parent::ParentMethod);
  }
};

template<>
struct Type<Child> {
  using Base = Parent;
  static constexpr const char* name = "Child";
  static Child* Constructor() {
    return new Child();
  }
  static void Define(napi_env env, napi_value, napi_value prototype) {
    Set(env, prototype, "childMethod", &Child::ChildMethod);
  }
};

template<typename T>
struct TypeBridge<T, typename std::enable_if<std::is_base_of<
                         Parent, T>::value>::type> {
  static T* Wrap(T* ptr) {
    return ptr;
  }
  static void Finalize(T* ptr) {
    delete ptr;
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

template<>
struct Type<Copiable> : public AllowPassByValue<Copiable> {
  static constexpr const char* name = "Copiable";
  static Copiable* Constructor() {
    return new Copiable;
  }
  static void Define(napi_env env, napi_value constructor, napi_value) {
    Set(env, constructor, "count", &Copiable::Count);
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
          "passThroughRefCounted", &PassThrough<RefCounted*>);

  ki::Set(env, binding,
          "Child", ki::Class<Child>(),
          "Parent", ki::Class<Parent>(),
          "childToParent", &ChildToParent,
          "pointerOfParent", &PointerOf<Parent>,
          "pointerOfChild", &PointerOf<Child>);

  WeakFactory* factory = new WeakFactory;
  ki::Set(env, binding,
          "weakFactory", factory,
          "WeakFactory", ki::Class<WeakFactory>());

  ki::Set(env, binding,
          "Copiable", ki::Class<Copiable>(),
          "passThroughCopiable", &PassThrough<Copiable>);
}
