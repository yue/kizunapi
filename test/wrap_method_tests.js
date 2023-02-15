const {runInNewScope, gcUntil} = require('./util')

exports.runTests = async (assert, binding, {addFinalizer, getAttachedTable}) => {
  const {View} = binding

  await runInNewScope(async () => {
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
  })

  await runInNewScope(async () => {
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
  })

  await runInNewScope(async () => {
    let parent = new View
    let viewCollected, childCollected
    runInNewScope(() => {
      const view = new View
      const child = new View
      parent.doNothingWithView(view)
      parent.addChildView(child)
      addFinalizer(view, () => viewCollected = true)
      addFinalizer(child, () => childCollected = true)
    })
    await gcUntil(() => viewCollected)
    assert.equal(childCollected, undefined,
                 'WrapMethod referenced data member does not get GCed')
  })

  await runInNewScope(async () => {
    const parent = new View
    let childCollected
    runInNewScope(() => {
      const child = new View
      parent.addChildView(child)
      parent.removeChildView(child)
      addFinalizer(child, () => childCollected = true)
    })
    await gcUntil(() => childCollected)
    assert.equal(childCollected, true,
                 'WrapMethod dereferenced data member get GCed')
  })
}
