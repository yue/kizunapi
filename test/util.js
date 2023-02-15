module.exports = {runInNewScope, gcUntil}

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
