#!/usr/bin/env node

import cp from 'child_process'
import fs from 'fs'
import path from 'path'
import url from 'url'
import extract from 'extract-zip'
import fetch from 'node-fetch'

const gnVersion = 'v0.8.2'

const __filename = url.fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)
const gnDir = path.join(__dirname, '..', 'deps', 'gn')

if (!fs.existsSync(gnDir))
  await downloadGN()

if (!fs.existsSync('out'))
  runSync(path.join(gnDir, 'gn'), ['gen', 'out'])

runSync(path.join(gnDir, 'ninja'), ['-C', 'out'])
runSync(path.join('out', 'nb_tests'))

function runSync(exec, args = [], options = {}) {
  // Print command output by default.
  if (!options.stdio)
    options.stdio = 'inherit'
  // Merge the custom env to global env.
  if (options.env)
    options.env = Object.assign(options.env, process.env)
  const result = cp.spawnSync(exec, args, options)
  if (result.error)
    throw result.error
  if (result.signal)
    throw new Error(`Process aborted with ${result.signal}`)
  return result
}

async function downloadGN() {
  const os = {
    linux: 'linux',
    win32: 'win',
    darwin: 'mac',
  }[process.env.npm_config_platform || process.platform]
  const gnUrl = `https://github.com/yue/build-gn/releases/download/${gnVersion}/gn_${gnVersion}_${os}_x64.zip`

  const res = await fetch(gnUrl)
  await new Promise((resolve, reject) => {
    const stream = fs.createWriteStream('gn.zip')
    res.body.pipe(stream)
    res.body.on('error', reject)
    stream.on('finish', resolve)
  })
  await extract('gn.zip', {dir: gnDir})
  fs.unlinkSync('gn.zip')
}
