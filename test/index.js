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
      assert, bindings[test], {addFinalizer, getAttachedTable})
  }
}
