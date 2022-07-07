exports.runTests = (assert, binding) => {
  assert.equal(binding.value, 'value',
               'Property value')
  delete binding.value
  assert.equal(binding.value, undefined,
               'Property value defaults to configurable')

  assert.equal(binding.method1(), 8964, 'Property method')
  assert.equal(binding.method2(), binding.method3(),
               'Property method define helpers')
  assert.equal(binding.propertyIsEnumerable('method'), false,
               'Property method defaults to not enumerable')

  assert.equal(binding.number, 19890604, 'Property getter')
  binding.number = 89
  assert.equal(binding.number, 90, 'Property setter')
  delete binding.number
  assert.equal(binding.number, 90,
               'Property setter defaults to not configurable')

  const {member} = binding
  assert.equal(member.getter, 89,
               'Property member data pointer to getter')
  member.setter = 64
  assert.equal(member.getter, 64,
               'Property member data pointer to setter')
  member.data = 8964
  assert.equal(member.data, 8964,
               'Property member data pointer to getter and setter')
}
