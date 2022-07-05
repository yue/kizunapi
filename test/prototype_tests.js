exports.runTests = (assert, binding) => {
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
  assert.ok(pointerOfClass(classWithConstructor) > 0,
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
  assert.ok(refCounted instanceof RefCounted,
            'Prototype push pointer to js')
  assert.equal(passThroughRefCounted(refCounted), refCounted,
               'Prototype push pointer to native')

  const {pointerOfChild, pointerOfParent, Child, Parent} = binding
  const child = new Child
  const parent = new Parent
  assert.ok(child instanceof Parent,
            'Prototype native inheritance maps to js')
  assert.ok(pointerOfParent(child) > 0,
            'Prototype child can convert to parent in native')
  assert.throws(() => { pointerOfChild(parent) },
                {
                  name: 'TypeError',
                  message: 'Error processing argument at index 0, conversion failure from Parent to Child.',
                },
                'Prototype parent can not convert to child')
}
