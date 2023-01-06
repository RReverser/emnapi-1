/* eslint-disable no-new-object */
/* eslint-disable no-new-wrappers */
/* eslint-disable camelcase */
'use strict'
const assert = require('assert')
const { load } = require('../util')

module.exports = load('emnapitest').then(test_typedarray => {
  const mod = test_typedarray.getModuleObject()

  const HEAPU8 = test_typedarray.getModuleProperty('HEAPU8')
  const mem = HEAPU8.buffer
  assert.ok(mem instanceof ArrayBuffer || mem instanceof SharedArrayBuffer)
  assert.strictEqual(mod.HEAPU8, HEAPU8)

  const externalResult = test_typedarray.External()
  assert.ok(externalResult instanceof Uint8Array)
  assert.deepStrictEqual([...externalResult], [0, 1, 2])
  test_typedarray.GrowMemory()
  assert.deepStrictEqual([...externalResult], [0, 1, 2])

  const buffer = test_typedarray.NullArrayBuffer()
  assert.ok(buffer instanceof Uint8Array)
  assert.strictEqual(buffer.length, 0)
  assert.strictEqual(buffer.buffer, mem)
  assert.notStrictEqual(buffer.buffer.byteLength, 0)

  const [major, minor, patch] = test_typedarray.testGetEmscriptenVersion()
  assert.strictEqual(typeof major, 'number')
  assert.strictEqual(typeof minor, 'number')
  assert.strictEqual(typeof patch, 'number')
  console.log(`test: Emscripten v${major}.${minor}.${patch}`)
})
