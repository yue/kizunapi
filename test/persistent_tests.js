const {gcUntil} = require('./util')

exports.runTests = async (assert, binding) => {
  const {PersistentMap} = binding
  const map = new PersistentMap

  let object = {}
  map.set(1, object)
  map.makeWeak(1)
  object = undefined
  assert.doesNotReject(async () => {
    await gcUntil(() => map.get(1) == undefined)
  }, 'Persistent gc removes weak refed object')
}
