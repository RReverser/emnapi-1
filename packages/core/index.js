if (typeof process !== 'undefined' && process.env.NODE_ENV === 'production') {
  module.exports = require('./dist/emnapi-core.cjs.min.js')
} else {
  module.exports = require('./dist/emnapi-core.cjs.js')
}
