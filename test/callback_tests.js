exports.runTests = async (assert, binding, {runInNewScope, gcUntil, addFinalizer}) => {
  assert.deepStrictEqual(binding.returnVoid(), undefined,
                         'Callback void return value converts to undefined')
  assert.equal(binding.addOne(123), 124, 'Callback convert arg from js')

  assert.throws(() => { binding.addOne('string') },
                {
                  name: 'TypeError',
                  message: 'Error processing argument at index 0, conversion failure from String to Integer.',
                },
                'Callback throw when arg type does not match')

  assert.equal(binding.append64(() => '89'), '8964',
               'Callback convert js function to std::function')

  assert.equal(binding.nullFunction, null,
               'Callback convert null function to null')

  binding.method.call(binding.object, 1)
  assert.equal(binding.data.call(binding.object), 8964,
               'Callback convert member function to js')

  assert.throws(() => { binding.method() },
                {
                  name: 'TypeError',
                  message: 'Error converting "this" to TestClass.',
                },
                'Callback throw when |this| does not match member function')

  await runInNewScope(async () => {
    let someFunctionCollected = false
    runInNewScope(() => {
      const someFunction = () => {}
      binding.storeWeakFunction(someFunction);
      addFinalizer(someFunction, () => someFunctionCollected = true)
    })
    await gcUntil(() => someFunctionCollected)
    assert.throws(() => { binding.runStoredFunction() },
                  {
                    name: 'Error',
                    message: 'The function has been garbage collected',
                  },
                  'Callback throw when weak function has been garbage collected')
    binding.clearStoredFunction()
  })
}
