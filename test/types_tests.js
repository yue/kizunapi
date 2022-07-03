exports.runTests = (assert, binding) => {
  assert.deepEqual(binding.value, 'value', 'ToNode value')
  assert.deepEqual(binding.null, null, 'ToNode null')
  assert.deepEqual(binding.integer, 123, 'ToNode integer')
  assert.deepEqual(binding.number, 3.14, 'ToNode number')
  assert.deepEqual(binding.bool, false, 'ToNode bool')
  assert.deepEqual(binding.string, '字符串', 'ToNode string')
  assert.deepEqual(binding.ustring, 'ustring', 'ToNode ustring')
  assert.deepEqual(binding.charptr, 'チャーポインター', 'ToNode charptr')
  assert.deepEqual(binding.ucharptr, 'ucharptr', 'ToNode ucharptr')
}
