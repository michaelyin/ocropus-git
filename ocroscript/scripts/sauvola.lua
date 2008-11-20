require 'lib.util'

import_all(iulib)
import_all(ocr)

if #arg < 2 then
    print("usage: ... [--k=value] [--w=value] input output")
    os.exit(1)
end

binarizer = ocr.make_BinarizeBySauvola()

opt,arg = util.getopt(arg)

if (opt.w ~= nil) then
    binarizer:set("w",tonumber(opt.w)) 
end
if (opt.k ~= nil) then
    binarizer:set("k",tonumber(opt.k)) 
end

input = bytearray()
output = bytearray()

iulib.read_image_gray(input,arg[1])
binarizer:binarize(output,input)
iulib.write_image_gray(arg[2],output)
