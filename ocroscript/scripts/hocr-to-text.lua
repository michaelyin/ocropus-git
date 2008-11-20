require 'lib.hocr'
require 'lib.util'
require 'lib.xml'

if #arg ~= 1 then
    print("Usage: ocroscript hocr-to-text input.hocr >output.txt")
    os.exit(1)
end

-- CK: the script should expect correct XHTML and not convert it implicitly
-- local f = io.popen(('tidy -q -asxml -utf8 "%s"'):format(arg[1]))
local f = io.open(arg[1])
local hocr = xml.collect(f:read('*a'))
f:close()
for page_no, page in pairs(xml.get_list_of_subitems_by_DOM(hocr, 'ocr_page')) do
    for i, line_DOM in pairs(xml.get_list_of_subitems_by_DOM(page, 'ocr_line')) do
        print((xml.get_transcript_by_line_DOM(line_DOM):gsub('%s*$','')))
    end
end
