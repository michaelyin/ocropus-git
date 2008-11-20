-- Align hOCR output with images in order to generate training data.

require 'lib.util'
require 'lib.xml'
require 'lib.align'

if #arg ~= 2 then
    print("Usage: "..arg[0].." <input hocr> <output dir>")
    os.exit(2)
end

bpnet = make_NewBpnetLineOCR('../data/models/neural-net-file.nn')
align.align_book(bpnet, arg[1], arg[2])
