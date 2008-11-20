-- clean up text line images by removing connected components
-- that are "too small" and removing connected components that
-- don't overlap the center of the image
--
-- this is mostly useful for removing bits of characters intruding
-- into a line image from neighboring lines

-- require "lib.getopt"
require "lib.util"
import_all(ocr)

if #arg < 2 then
    print("usage: ocroscript "..arg[0].." input output")
    os.exit(1)
end

image = bytearray()
iulib.read_image_binary(image,arg[1])
narray.sub(255,image)
remove_small_components(image,3,3)
d = math.floor(image:dim(1)/4)
if d>0 then remove_marginal_components(image,4,d,4,d) end
narray.sub(255,image)
iulib.write_image_gray(arg[2],image)
