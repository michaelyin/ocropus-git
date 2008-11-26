import_all(iulib)
import_all(ocr)
import_all(layout)

image = bytearray()
timap = intarray()
obstacles = rectarray()
result1 = intarray()
result2 = intarray()

segmenter = make_SegmentPageByRAST() 
--textimageseg = make_TextImageSegByLeptonica() 
textimageseg = make_TextImageSegByLogReg() 

read_image_gray(image,arg[1])
textimageseg:textImageProbabilities(timap,image)
write_image_packed("tiseg.png",timap)
get_nontext_boxes(obstacles,timap)
---[[
--segmenter:segment(result1,image)
--check_page_segmentation(result1)
--simple_recolor(result1)
--write_image_packed(arg[2],result1)

segmenter:segment(result2,image,obstacles)
check_page_segmentation(result2)
write_image_packed(arg[2],result2)
simple_recolor(result2)
write_image_packed(arg[3],result2)
--]]