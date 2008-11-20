import_all(graphics)
-- -*- lua -*-

dofile("utest.lua")

-- dinit(800,800)

line_image = bytearray:new() line_seg = intarray:new()

iulib.read_image_gray(line_image,"images/line.png")
iulib.make_page_black(line_image)

dshow(line_image,"y")

linesegmenter = ocr.make_ConnectedComponentSegmenter()
linesegmenter:charseg(line_seg,line_image)
grouper = ocr.make_SimpleGrouper()
ocr.sort_by_xcenter(line_seg)
grouper:setSegmentation(line_seg)

char = bytearray:new()
mask = bytearray:new()

for i=0,grouper:length()-1 do
   grouper:extract(char,mask,line_image,i)
   test_assert(char:dim(0)>0 and char:dim(1)>0,"zero char output")
   test_assert(char:dim(0)==mask:dim(0) and char:dim(1)==mask:dim(1),
               "char size != mask size")
   dshow(mask,"c")
   dshow(char,"d")
   dwait()
end
