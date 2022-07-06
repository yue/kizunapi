exports.runTests = (assert, binding) => {
  assert.equal(binding.value, 'value',
               'Property value')
  delete binding.value
  assert.equal(binding.value, undefined,
               'Property value defaults to configurable')

  assert.equal(binding.method(), 8964, 'Property method')
  assert.equal(binding.propertyIsEnumerable('method'), false,
               'Property method defaults to not enumerable')

  assert.equal(binding.number, 19890604, 'Property getter')
  binding.number = 89
  assert.equal(binding.number, 90, 'Property setter')
  delete binding.number
  assert.equal(binding.number, 90,
               'Property setter defaults to not configurable')
}
