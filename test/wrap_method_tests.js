const {runInNewScope, gcUntil} = require('./util')

exports.runTests = async (assert, binding, {addFinalizer}) => {
  const {View} = binding

  let parentCollected, childCollected
  runInNewScope(() => {
    const parent = new View
    const child = new View
    parent.addChildView(child)
    addFinalizer(parent, () => parentCollected = true)
    addFinalizer(child, () => childCollected = true)
  })
  await gcUntil(() => childCollected)
  assert.equal(parentCollected && childCollected, true,
               'WrapMethod parent with child get GCed')

  let viewCollected, listenerCollected
  runInNewScope(() => {
    const view = new View
    const listener = () => {}
    view.addEventListener(listener)
    addFinalizer(view, () => viewCollected = true)
    addFinalizer(listener, () => listenerCollected = true)
  })
  await gcUntil(() => listenerCollected)
  assert.equal(viewCollected && listenerCollected, true,
               'WrapMethod object with callback get GCed')
}
