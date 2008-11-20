dofile("utest.lua")

image = intarray(500,500)

narray.fill(image,0)
image:put(10,10,1)
image:put(10,20,2)
image:put(10,30,3)

r = ocr.RegionExtractor()
r:setImage(image)
test_eq(4,r:length(),"setImage")

r = ocr.RegionExtractor()
r:setImageMasked(image)
test_eq(4,r:length(),"setImageMasked")

narray.fill(image,0)
image:put(10,10,ocr.pseg_pixel(1,1,1))
image:put(10,20,ocr.pseg_pixel(2,1,1))
image:put(10,30,ocr.pseg_pixel(3,1,1))
image:put(10,40,ocr.pseg_pixel(4,1,1))
image:put(10,50,ocr.pseg_pixel(4,1,2))
image:put(10,60,ocr.pseg_pixel(4,1,3))

r = ocr.RegionExtractor()
r:setPageColumns(image)
test_eq(5,r:length(),"setColumns")
