const fs = require('fs')
const path = require('path')
const assert = require('tapsert')

const bindings = require('./build/Debug/ki_tests')

main().catch(e => {
  console.log(e)
  process.exit(1)
})

async function main() {
  const {addFinalizer, getAttachedTable} = bindings
  for (const f of fs.readdirSync(__dirname)) {
    if (!f.endsWith('_tests.js'))
      continue
    const test = path.basename(f, '_tests.js')
    await require(path.join(__dirname, f)).runTests(
      assert, bindings[test], {runInNewScope, gcUntil, addFinalizer, getAttachedTable})
  }
}

async function runInNewScope(func) {
  await (async function() {
    await func()
  })()
}

function gcUntil(condition) {
  return new Promise((resolve, reject) => {
    let count = 0
    function gcAndCheck() {
      setImmediate(() => {
        count++
        gc()
        if (condition()) {
          resolve()
        } else if (count < 10) {
          gcAndCheck()
        } else {
          reject('GC failure')
        }
      })
    }
    gcAndCheck()
  })
}
