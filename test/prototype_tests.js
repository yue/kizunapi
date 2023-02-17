exports.runTests = async (assert, binding, {runInNewScope, gcUntil, addFinalizer}) => {
  const {SimpleClass} = binding
  assert.throws(() => { new SimpleClass },
                {
                  name: 'Error',
                  message: 'There is no constructor defined.',
                },
                'Prototype throw when creating class without constructor')

  const {pointerOfClass, ClassWithConstructor} = binding
  const classWithConstructor = new ClassWithConstructor
  assert.equal(classWithConstructor.constructor.name, 'ClassWithConstructor',
               'Prototype can create class with constructor')
  assert.equal(pointerOfClass(classWithConstructor) > 0, true,
               'Prototype can pass class with constructor to native')
  assert.throws(() => { ClassWithConstructor() },
                {
                  name: 'Error',
                  message: 'Constructor must be called with new.',
                },
                'Prototype throw when invoking constructor without new')

  const {ThrowInConstructor} = binding
  assert.throws(() => { new ThrowInConstructor() },
                {
                  name: 'Error',
                  message: 'Throwed in constructor',
                },
                'Prototype can throw in constructor')

  const {passThroughRefCounted, refCounted, RefCounted} = binding
  assert.equal(refCounted instanceof RefCounted, true,
               'Prototype push pointer to js')
  assert.equal(refCounted.count(), 1,
               'Prototype call wrap when pushing pointer to js')
  assert.equal(passThroughRefCounted(refCounted), refCounted,
               'Prototype push pointer to native')
  assert.equal(refCounted.count(), 1,
               'Prototype wrap is only called once per object')
  assert.equal((new RefCounted).count(), 1,
               'Prototype constructor and wrap work together')

  const {pointerOfChild, pointerOfParent, Child, Parent} = binding
  const child = new Child
  const parent = new Parent
  assert.equal(child instanceof Parent, true,
               'Prototype native inheritance maps to js')
  assert.equal(pointerOfParent(child) > 0, true,
               'Prototype child can convert to parent in native')
  assert.throws(() => { pointerOfChild(parent) },
                {
                  name: 'TypeError',
                  message: 'Error processing argument at index 0, conversion failure from Parent to Child.',
                },
                'Prototype parent can not convert to child')
  assert.equal(child.parentMethod(), 89,
               'Prototype child can call parent method')

  const {weakFactory, WeakFactory} = binding
  weakFactory.destroy()
  assert.throws(() => { weakFactory.destroy() },
                {
                  name: 'TypeError',
                  message: 'Error converting "this" to WeakFactory.',
                },
                'Prototype wrap and unwrap internal pointer from C++')

  let weakFactoryCollected
  runInNewScope(() => {
    const f = new WeakFactory
    addFinalizer(f, () => weakFactoryCollected = true)
  })
  await gcUntil(() => weakFactoryCollected)
  assert.ok(true, 'Prototype wrap and unwrap internal pointer from js')
}
