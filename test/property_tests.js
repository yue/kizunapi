exports.runTests = async (assert, binding, {runInNewScope, gcUntil, addFinalizer}) => {
  assert.equal(binding.value, 'value',
               'Property value')
  delete binding.value
  assert.equal(binding.value, undefined,
               'Property value defaults to configurable')

  assert.equal(binding.number, 19890604, 'Property getter')
  binding.number = 89
  assert.equal(binding.number, 90, 'Property setter')
  delete binding.number
  assert.equal(binding.number, 90,
               'Property setter defaults to not configurable')

  let callbackCollected
  runInNewScope(() => {
    const callback = () => {}
    binding.member.callback = callback
    addFinalizer(callback, () => callbackCollected = true)
  })
  await gcUntil(() => callbackCollected)
  assert.equal(callbackCollected, true,
               'Property setter converts callback to weak function')

  const {member} = binding
  assert.equal(member.getter, 89,
               'Property member data pointer to getter')
  member.setter = 64
  assert.equal(member.getter, 64,
               'Property member data pointer to setter')
  member.data = 8964
  assert.equal(member.data, 8964,
               'Property member data pointer to getter and setter')

  const {HasObjectMember} = binding
  const has = new HasObjectMember
  assert.equal(has.member.data, 89,
               'Property object pointer property to js')
  has.member = member
  assert.equal(has.member.data, 8964,
               'Property change object pointer property')

  const life = new HasObjectMember
  life.member.customData = 123
  assert.doesNotReject(async () => {
    await gcUntil(() => life.member.customData === undefined)
  }, 'Property JS wrapper of property gets GCed')

  life.strong.customData = 123
  life.member.customData = 123
  await gcUntil(() => life.member.customData === undefined)
  assert.equal(life.strong.customData, 123,
               'Property cached property does not get GCed')
}
