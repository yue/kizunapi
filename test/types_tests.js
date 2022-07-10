exports.runTests = (assert, binding) => {
  assert.equal(binding.value, 'value', 'ToNode value')
  assert.strictEqual(binding.null, null, 'ToNode null')
  assert.equal(binding.integer, 123, 'ToNode integer')
  assert.equal(binding.number, 3.14, 'ToNode number')
  assert.equal(binding.bool, false, 'ToNode bool')
  assert.equal(binding.string, '字符串', 'ToNode string')
  assert.equal(binding.ustring, 'ustring', 'ToNode ustring')
  assert.equal(binding.charptr, 'チャーポインター', 'ToNode charptr')
  assert.equal(binding.ucharptr, 'ucharptr', 'ToNode ucharptr')
  assert.equal(typeof binding.symbol, 'symbol', 'ToNode symbol')
}
