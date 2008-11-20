require 'lib.path'

assert(path)
import_all(path)

assert(isabs '/abc')
assert(not isabs 'abc')
assert(not isabs 'abc/def')

assert(isdir 'images')
assert(isdir '.')
assert(not isdir 'test-path.lua')

assert(exists 'images')
assert(exists '.')
assert(exists 'test-path.lua')
assert(not exists 'bug-free software')

assert(path.join('a') == 'a')
assert(path.join('a', 'b') == 'a/b')
assert(path.join('a', 'b', 'c') == 'a/b/c')
assert(path.join('a', '/b', 'c') == '/b/c')
assert(path.join('a', '/b/', 'c') == '/b/c')

assert(path.dirname('a') == '.')
assert(path.dirname('abc/cde') == 'abc/')
assert(path.dirname('abc/cde/') == 'abc/cde/')

assert(path.search('.:images', 'line.png') == 'images/line.png')
assert(path.search('.:images', 'test-path.lua') == 'test-path.lua')
assert(not path.search('.:images', 'meaning of life'))
