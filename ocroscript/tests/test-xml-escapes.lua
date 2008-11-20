import_all(nustring)

local function unescape(str)
    local n = nustring()
    ocr.xml_unescape(n, str)
    return n:utf8()
end

assert(unescape '&lt;&gt;' == '<>')
assert(unescape '&amp; &apos; &quot;' == '& \' "')
assert(unescape '&#65;' == 'A')
assert(unescape '&#x20;' == ' ')
